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
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.LogDataType;
import java.io.File;

/**
 * Utility class to add output file to TradeFed log directory.
 */
public class OutputUtil {
    // Test logger object from test invocation
    ITestLogger mListener;

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
}