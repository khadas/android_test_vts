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

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;

import junit.framework.TestCase;
import org.easymock.EasyMock;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

/**
 * Unit tests for {@link VtsMultiDeviceTest}.
 */
public class VtsMultiDeviceTestResultParserTest extends TestCase {

    private ITestInvocationListener mMockInvocationListener = null;
    private ITestDevice mMockITestDevice = null;
    private IRunUtil mRunUtil = null;
    private VtsMultiDeviceTest mTest = null;
    private static final String TEST_CASE_PATH =
        "test/vts/testcases/host/sample/SampleLightTest";
    private static final String TEST_CONFIG_PATH =
        "test/vts/testcases/host/sample/SampleLightTest.config";
    static final long TEST_TIMEOUT = 1000 * 60 * 5;
    private String[] mCommand = null;
    // for file path
    private static final String TEST_DIR = "tests";
    private static final String OUTPUT_FILE_1 = "vts_multi_device_test_parser_output.txt";
    private static final String OUTPUT_FILE_2 = "vts_multi_device_test_parser_output_timeout.txt";
    private static final String USER_DIR = "user.dir";
    private static final String RES = "res";
    private static final String TEST_TYPE = "testtype";

    /**
     * Helper to initialize the various EasyMocks we'll need.
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTest = new VtsMultiDeviceTest();
        mRunUtil = EasyMock.createMock(IRunUtil.class);
        mMockInvocationListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockITestDevice = EasyMock.createMock(ITestDevice.class);
        mTest.setRunUtil(mRunUtil);
        //build the command
        String[] baseOpts = {"/usr/bin/python", "-m"};
        String[] testModule = {TEST_CASE_PATH, TEST_CONFIG_PATH};
        mCommand = ArrayUtil.buildArray(baseOpts, testModule);
    }

    /**
     * Test the run method with a normal input.
     * @throws IOException
     */
    public void testRunTimeoutInput() throws IOException{
        mTest.setDevice(mMockITestDevice);
        mTest.setTestCasePath(TEST_CASE_PATH);
        mTest.setTestConfigPath(TEST_CONFIG_PATH);

        CommandResult commandResult = new CommandResult();
        commandResult.setStatus(CommandStatus.SUCCESS);
        commandResult.setStdout(getOutputTimeout());
        EasyMock.expect(mRunUtil.runTimedCmd(TEST_TIMEOUT, mCommand)).
                andReturn(commandResult);
        EasyMock.replay(mRunUtil);
        try {
            mTest.run(mMockInvocationListener);
        } catch (IllegalArgumentException e) {
            // not expected
            fail();
            e.printStackTrace();
        } catch (DeviceNotAvailableException e) {
            // not expected
            fail();
            e.printStackTrace();
        }
    }
    /**
     * Returns the sample shell output for a test command.
     * @return {String} shell output
     * @throws IOException
     */
    private String getOutputTimeout() throws IOException{
        BufferedReader br = null;
        String output = null;
        try {
            br = new BufferedReader(new FileReader(getFileNameTimeout()));
            StringBuilder sb = new StringBuilder();
            String line = br.readLine();

            while (line != null) {
                sb.append(line);
                sb.append(System.lineSeparator());
                line = br.readLine();
            }
            output = sb.toString();
        } finally {
            br.close();
        }
        return output;
    }

    /** Return the file path that contains sample shell output logs.
     * @return {String} file path
     */private String getFileNameTimeout(){
           StringBuilder path = new StringBuilder();
           path.append(System.getProperty(USER_DIR)).append(File.separator).append(TEST_DIR).
                   append(File.separator).append(RES).append(File.separator).
                   append(TEST_TYPE).append(File.separator).append(OUTPUT_FILE_2);
           return path.toString();
     }
     /**
      * Test the run method with a normal input.
      * @throws IOException
      */
     public void testRunNormalInput() throws IOException{
         mTest.setDevice(mMockITestDevice);
         mTest.setTestCasePath(TEST_CASE_PATH);
         mTest.setTestConfigPath(TEST_CONFIG_PATH);

         CommandResult commandResult = new CommandResult();
         commandResult.setStatus(CommandStatus.SUCCESS);
         commandResult.setStdout(getOutput());
         EasyMock.expect(mRunUtil.runTimedCmd(TEST_TIMEOUT, mCommand)).
                 andReturn(commandResult);
         EasyMock.replay(mRunUtil);
         try {
             mTest.run(mMockInvocationListener);
         } catch (IllegalArgumentException e) {
             // not expected
             fail();
             e.printStackTrace();
         } catch (DeviceNotAvailableException e) {
             // not expected
             fail();
             e.printStackTrace();
         }
     }
     /**
      * Returns the sample shell output for a test command.
      * @return {String} shell output
      * @throws IOException
      */
     private String getOutput() throws IOException{
         BufferedReader br = null;
         String output = null;
         try {
             br = new BufferedReader(new FileReader(getFileName()));
             StringBuilder sb = new StringBuilder();
             String line = br.readLine();

             while (line != null) {
                 sb.append(line);
                 sb.append(System.lineSeparator());
                 line = br.readLine();
             }
             output = sb.toString();
         } finally {
             br.close();
         }
         return output;
     }

     /** Return the file path that contains sample shell output logs.
      * @return {String} file path
      */private String getFileName(){
            StringBuilder path = new StringBuilder();
            path.append(System.getProperty(USER_DIR)).append(File.separator).append(TEST_DIR).
                    append(File.separator).append(RES).append(File.separator).
                    append(TEST_TYPE).append(File.separator).append(OUTPUT_FILE_1);
            return path.toString();
      }
}
