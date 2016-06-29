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

/**
 * Unit tests for {@link VtsMultiDeviceTest}.
 */
public class VtsMultiDeviceTestTest extends TestCase {

    private ITestInvocationListener mMockInvocationListener = null;
    private ITestDevice mMockITestDevice = null;
    private IRunUtil mRunUtil = null;
    private VtsMultiDeviceTest mTest = null;
    private static final String TEST_CASE_PATH =
        "test/vts/testcases/host/sample/SampleLightTest";
    private static final String TEST_CONFIG_PATH =
        "test/vts/testcases/host/sample/SampleLightTest.config";
    private static long TEST_TIMEOUT = 1000 * 60 * 5;
    private String[] mCommand = null;

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
     */
    public void testRunNormalInput(){
        mTest.setDevice(mMockITestDevice);
        mTest.setTestCasePath(TEST_CASE_PATH);
        mTest.setTestConfigPath(TEST_CONFIG_PATH);

        CommandResult commandResult = new CommandResult();
        commandResult.setStatus(CommandStatus.SUCCESS);
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
     * Test the run method when the device is set null.
     */
    public void testRunDevice(){
        mTest.setDevice(null);
        try {
            mTest.run(mMockInvocationListener);
            fail();
       } catch (IllegalArgumentException e) {
            // not expected
            fail();
       } catch (DeviceNotAvailableException e) {
            // expected
       }
    }

    /**
     * Test the run method with abnormal input data.
     */
    public void testRunNormalInputCommandFailed() {
        mTest.setDevice(mMockITestDevice);
        CommandResult commandResult = new CommandResult();
        commandResult.setStatus(CommandStatus.FAILED);
        EasyMock.expect(mRunUtil.runTimedCmd(TEST_TIMEOUT, mCommand)).
            andReturn(commandResult);
        EasyMock.replay(mRunUtil);
        try {
            mTest.run(mMockInvocationListener);
            fail();
       } catch (RuntimeException e) {
             // expected
       } catch (DeviceNotAvailableException e) {
             // not expected
             fail();
             e.printStackTrace();
       }
    }
}
