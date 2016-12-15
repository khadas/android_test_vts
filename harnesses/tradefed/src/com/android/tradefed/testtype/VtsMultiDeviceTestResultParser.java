/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.tradefed.testtype;

import com.android.ddmlib.MultiLineReceiver;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.log.LogUtil.CLog;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Interprets the output of tests run with Python's unit test framework and translates it into
 * calls on a series of {@link ITestRunListener}s. Output from these tests follows this
 * EBNF grammar:
 */
public class VtsMultiDeviceTestResultParser extends MultiLineReceiver {

    // Parser state
    String[] mAllLines;
    String mCurrentLine;
    int mLineNum;
    ParserState mCurrentParseState = ParserState.TEST_CASE_ENTRY;

    // Current test state
    String mTestClass;
    TestIdentifier mCurrentTestId;
    String mCurrentTraceback;
    long mTotalElapsedTime = 0;

    // General state
    private Map<TestIdentifier, String> mTestResultCache;
    private int mFailedTestCount = 0;
    private final Collection<ITestRunListener> mListeners;
    private String mRunName = null;
    private String mCurrentTestName = null;
    private int mTotalTestCount = 0;

    // Variables to keep track of state for unit test
    private int mNumTestsRun = 0;
    private int mNumTestsExpected = 1;

    // Constant tokens that appear in the result grammar.
    static final String CASE_OK = "ok";
    static final String RUN_OK = "OK";
    static final String RUN_FAILED = "FAILED";
    static final String TEST_CASE = "[Test Case]";
    static final String PASS = "PASS";
    static final String FAIL = "FAIL";
    static final String TIMEOUT = "TIMEOUT";
    static final String END_PATTERN = "<==========";
    static final String BEGIN_PATTERN = "==========>";

    // Enumeration for parser state.
    static enum ParserState {
        TEST_CASE_ENTRY,
        COMPLETE,
        TEST_RUN
  }

    private class PythonUnitTestParseException extends Exception {
        public PythonUnitTestParseException(String reason) {
            super(reason);
        }
    }

    public VtsMultiDeviceTestResultParser(ITestRunListener listener,
        String runName) {
        mListeners = new ArrayList<ITestRunListener>(1);
        mListeners.add(listener);
        mRunName = runName;
        mTestResultCache = new HashMap<>();
    }

    @Override
    public void processNewLines(String[] lines) {
        init(lines);

        // extract the test class name if pattern matches for the first line
        String[] toks = mCurrentLine.split(" ");
        if (toks.length < 3 || !toks[toks.length - 1].equals(END_PATTERN) || !toks[toks.length - 3].
            equals(BEGIN_PATTERN)) {
            try {
                parseError(BEGIN_PATTERN);
            } catch (PythonUnitTestParseException e) {
                e.printStackTrace();
            }
        } else {
            mTestClass = toks[toks.length - 2];
        }

        // parse all lines
        for (String line : lines) {
            if (line.length() == 0 || !line.contains(TEST_CASE)) {
                continue;
            }
            mCurrentLine = line;
            try {
                parse();
            } catch (PythonUnitTestParseException e) {
                e.printStackTrace();
            }
        }

        // current state should not be in TEST_RUN state; if it's then mark for timeout.
        if (mCurrentParseState.equals(ParserState.TEST_RUN)) {
            markTestTimeout();
        }
        summary(lines);
        completeTestRun();
        mNumTestsRun++;
    }

    /**
     * Called by parent when adb session is complete.
     */
    @Override
    public void done() {
        super.done();
        if (mNumTestsRun < mNumTestsExpected) {
            handleTestRunFailed(String.format("Test run failed. Expected %d tests, received %d",
                    mNumTestsExpected, mNumTestsRun));
        }
    }

    /**
     * Process an instrumentation run failure
     *
     * @param errorMsg The message to output about the nature of the error
     */
    private void handleTestRunFailed(String errorMsg) {
        errorMsg = (errorMsg == null ? "Unknown error" : errorMsg);
        CLog.e(String.format("Test run failed: %s", errorMsg));

        Map<String, String> emptyMap = Collections.emptyMap();
        for (ITestRunListener listener : mListeners) {
            listener.testFailed(mCurrentTestId, FAIL);
            listener.testEnded(mCurrentTestId, emptyMap);
        }

        // Report the test run failed
        for (ITestRunListener listener : mListeners) {
            listener.testRunFailed(errorMsg);
        }
    }

    void init(String[] lines) {
        mAllLines = lines;
        mLineNum = 0;
        mCurrentLine = mAllLines[0];
    }

    /**
     * This method parses individual lines and calls functions based on the parsing state.
     * @throws PythonUnitTestParseException
     */
    void parse() throws PythonUnitTestParseException {
      try {
          switch(mCurrentParseState) {
              case TEST_CASE_ENTRY:
                  registerTest();
                  mCurrentParseState = ParserState.TEST_RUN;
                  break;
              case TEST_RUN:
                  if (testRun()) {
                      mCurrentParseState = ParserState.COMPLETE;
                  } else {
                      // incomplete test due to timeout.
                      mCurrentParseState = ParserState.TEST_CASE_ENTRY;
                      parse();
                  }
                  break;
              case COMPLETE:
                  mCurrentParseState = ParserState.TEST_CASE_ENTRY;
                  parse();
                  break;
              default:
                  break;
          }
      } catch (ArrayIndexOutOfBoundsException e) {
            CLog.d("Underlying error in testResult: " + e);
            throw new PythonUnitTestParseException("FailMessage");
        }
    }

    /**
     * This is called whenever the parser state is {@link ParserState#TEST_RUN}
     */
    private boolean testRun() {
        // process the test case
        String[] toks = mCurrentLine.split(" ");
        // check the last token
        String lastToken = toks[toks.length - 1];
        if (lastToken.equals(PASS)){
            markTestSuccess();
            return true;
        } else if (lastToken.equals(FAIL)){
            markTestFailure();
            return true;
        } else {
            markTestTimeout();
            return false;
        }
    }

    /**
     * This method is called whenever the current test doesn't finish as expected and runs out
     * of time.
     */
    private void markTestTimeout() {
        mTestResultCache.put(mCurrentTestId, TIMEOUT);
    }

    /**
     * This method is called when parser is at {@link ParserState#TEST_CASE_ENTRY}} stage and
     * this registers a new test case.
     */
    private void registerTest() {
        // process the test case
        String[] toks = mCurrentLine.split(" ");
        mCurrentTestName = toks[toks.length - 1];
        mCurrentTestId = new TestIdentifier(mTestClass, mCurrentTestName);
        mTotalTestCount++;
    }
    /**
     * Called at the end of the parsing to calculate the {@link #mTotalElapsedTime}
     *
     * @param lines The corresponding logs.
     */
    void summary(String[] lines) {
        if (lines == null || lines.length == 0) {
            mTotalElapsedTime = 0;
            return;
        }

        Date startDate = getDate(lines, true);
        Date endDate = getDate(lines, false);

        if (startDate == null || endDate == null) {
            mTotalElapsedTime = 0;
            return;
        }
        mTotalElapsedTime = endDate.getTime() - startDate.getTime();
    }

    /**
     * Return the time in milliseconds to calculate the time elapsed in a particular test.
     *
     * @param lines The logs that need to be parsed.
     * @param calculateStartDate flag which is true if we need to calculate start date.
     * @return {Date} the start and end time corresponding to a test.
     */
    private Date getDate(String[] lines, boolean calculateStartDate) {
        Date date = null;
        int begin = calculateStartDate ? 0 : lines.length - 1;
        int diff = calculateStartDate ? 1 : -1;

        for (int index = begin; index >= 0 && index < lines.length; index += diff) {
            lines[index].trim();
            String[] toks = lines[index].split(" ");

            // set the start time from the first line
            // the loop should continue if exception occurs, else it can break
            if (toks.length < 3) {
                continue;
            }
            String time = toks[2];
            SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss.SSS");
            try {
                date = sdf.parse(time);
            } catch (ParseException e) {
                continue;
            }
            break;
        }
        return date;
    }

    boolean completeTestRun() {
        for (ITestRunListener listener: mListeners) {
            // do testRunStarted
            listener.testRunStarted(mCurrentTestName, mTotalTestCount);

            // mark each test passed or failed
            for (Entry<TestIdentifier, String> test : mTestResultCache.entrySet()) {
                listener.testStarted(test.getKey());
                if (test.getValue() == PASS) {
                    listener.testEnded(test.getKey(), Collections.<String, String>emptyMap());
                } else if (test.getValue() == TIMEOUT) {
                    listener.testFailed(test.getKey(), test.getValue());
                } else {
                    listener.testFailed(test.getKey(), test.getValue());
                }
            }
            listener.testRunEnded(mTotalElapsedTime, Collections.<String, String>emptyMap());
        }
        return true;
    }

    /**
     * This is called whenever the program encounters unexpected tokens in parsing.
     *
     * @param expected The string that was expected.
     * @throws PythonUnitTestParseException
     */
    private void parseError(String expected)
            throws PythonUnitTestParseException {
        throw new PythonUnitTestParseException(
            String.format("Expected \"%s\" on line %d, found %s instead",
                    expected, mLineNum + 1, mCurrentLine));
    }

    private void markTestSuccess() {
        mTestResultCache.put(mCurrentTestId, PASS);
    }

    private void markTestFailure() {
        mTestResultCache.put(mCurrentTestId, FAIL);
        mFailedTestCount++;
    }

    @Override
    public boolean isCancelled() {
        return false;
    }
}
