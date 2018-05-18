/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tradefed.util;

import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.LogDataType;
import java.io.File;
import java.io.IOException;

/**
 * Utility class to add output file to TradeFed log directory.
 */
public class OutputUtil {
    // Test logger object from test invocation
    ITestLogger mListener;
    private String mTestModuleName = null;
    private String mAbiName = null;

    // Python output file patterns to be included in results
    static private String[] PYTHON_OUTPUT_PATTERNS = new String[] {"test_run_details.*\\.txt",
            "vts_agent_.*\\.log", "systrace_.*\\.html", "logcat.*\\.txt", "bugreport.*\\.zip"};
    // Python folder pattern in which any files will be included in results
    static private String PYTHON_OUTPUT_ADDITIONAL = ".*additional_output_files";

    public OutputUtil(ITestLogger listener) {
        mListener = listener;
    }

    /**
     * Add a text file to log directory.
     * @param outputFileName output file base name.
     *                       The actual output name will contain a hash postfix.
     * @param source text file source
     */
    public void addOutputFromTextFile(String outputFileName, File source) {
        FileInputStreamSource inputSource = new FileInputStreamSource(source);
        mListener.testLog(outputFileName, LogDataType.TEXT, inputSource);
    }

    /**
     * Collect all VTS python runner log output files
     * @param logDirectory
     */
    public void collectVtsRunnerOutputs(File logDirectory) {
        // First, collect known patterns.
        for (String pattern : PYTHON_OUTPUT_PATTERNS) {
            try {
                FileUtil.findFiles(logDirectory, pattern)
                        .forEach(path -> addVtsRunnerOutputFile(new File(path)));
            } catch (IOException e) {
                CLog.e("Error reading log directory: %s", logDirectory);
                CLog.e(e);
            }
        }

        // Next, collect any additional files produced by tests.
        try {
            for (String path : FileUtil.findFiles(logDirectory, PYTHON_OUTPUT_ADDITIONAL)) {
                for (File f : new File(path).listFiles()) {
                    addVtsRunnerOutputFile(f);
                }

                // Only use the first result, if many were found.
                break;
            }
        } catch (IOException e) {
            CLog.e("Error reading log directory: %s", logDirectory);
            CLog.e(e);
        }
    }

    /**
     *
     * @param logFile
     */
    private void addVtsRunnerOutputFile(File logFile) {
        String fileName = logFile.getName();

        LogDataType type;
        if (fileName.endsWith(".html")) {
            type = LogDataType.HTML;
        } else if (fileName.startsWith("logcat")) {
            type = LogDataType.LOGCAT;
        } else if (fileName.startsWith("bugreport") && fileName.endsWith(".zip")) {
            type = LogDataType.BUGREPORTZ;
        } else if (fileName.endsWith(".txt") || fileName.endsWith(".log")) {
            type = LogDataType.TEXT;
        } else if (fileName.endsWith(".zip")) {
            type = LogDataType.ZIP;
        } else {
            CLog.w("Unknown output file type. Skipping %s", logFile);
            return;
        }

        String outputFileName = mTestModuleName + "_" + fileName + "_" + mAbiName;
        FileInputStreamSource inputSource = new FileInputStreamSource(logFile);
        mListener.testLog(outputFileName, type, inputSource);
    }

    /**
     * @param testModuleName
     */
    public void setTestModuleName(String testModuleName) {
        mTestModuleName = testModuleName;
    }

    /**
     * @param bitness
     */
    public void setAbiName(String abiName) {
        mAbiName = abiName;
    }
}