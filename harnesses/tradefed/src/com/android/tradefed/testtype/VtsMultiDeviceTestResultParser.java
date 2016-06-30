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
import com.android.tradefed.result.ITestInvocationListener;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Interprets the output of tests run with Python's unittest framework and translates it into
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
    long mTotalElapsedTime;

    // General state
    private Map<TestIdentifier, String> mTestResultCache;
    private int mFailedTestCount = 0;
    private final Collection<ITestInvocationListener> mListeners;
    private String mRunName;
    private int mTotalTestCount = 0;

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

    public VtsMultiDeviceTestResultParser(Collection<ITestInvocationListener> listeners,
        String runName) {
        mListeners = listeners;
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

        try {
            summary(lines);
        } catch (PythonUnitTestParseException e) {
            e.printStackTrace();
        }
        completeTestRun();
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
     * This function is called whenever the current test doesn't finish as expected and runs out
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
        mRunName = toks[toks.length - 1];
        mCurrentTestId = new TestIdentifier(mTestClass, mRunName);
        mTotalTestCount++;
    }

    void summary(String[] lines) throws PythonUnitTestParseException {
        String[] toks = lines[0].split(" ");
        // set the start time from the first line
        if (toks.length < 2) {
            parseError("Incorrect length");
        }
        String startTime = toks[2];

        // get end time
        toks = lines[lines.length - 1].split(" ");
        if (toks.length < 2) {
            parseError("Incorrect length");
        }
        String endTime = toks[2];
        SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss.SSS");
        Date startDate = null, enddate = null;
        try {
            startDate = sdf.parse(startTime);
            enddate = sdf.parse(endTime);
        } catch (ParseException e) {
            e.printStackTrace();
        }
        mTotalElapsedTime = enddate.getTime() - startDate.getTime();
    }

    boolean completeTestRun() {
        for (ITestRunListener listener: mListeners) {
            // do testRunStarted
            listener.testRunStarted(mRunName, mTotalTestCount);

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
