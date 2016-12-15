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

import com.android.ddmlib.IShellOutputReceiver;
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
    private IShellOutputReceiver mMockReceiver = null;
    private ITestDevice mMockITestDevice = null;
    private IRunUtil mRunUtil = null;
    private VtsMultiDeviceTest mTest = null;
    private static final String TEST_CASE_PATH = "test/vts/testcases/host/sample/SampleLightTest";
    private static final String TEST_CONFIG_PATH = "test/vts/testcases/host/sample/SampleLightTest.config";

    // This variable is set in order to include the directory that contains the python test cases.
    private String mPythonPath = null;

    private CommandResult mCommandResult = null;
    private String[] mCommand = null;


    /**
     * Helper to initialize the various EasyMocks we'll need.
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mRunUtil = EasyMock.createMock(IRunUtil.class);
        mMockInvocationListener = EasyMock.createMock(ITestInvocationListener.class);
        mMockReceiver = EasyMock.createMock(IShellOutputReceiver.class);
        mMockITestDevice = EasyMock.createMock(ITestDevice.class);
        mPythonPath = ":" + System.getenv("ANDROID_BUILD_TOP") + "/test";
        mTest = new VtsMultiDeviceTest();

        //build the command
        String[] baseOpts = {"python", "-m"};
        String[] testModule = {TEST_CASE_PATH, TEST_CONFIG_PATH};
        mCommand = ArrayUtil.buildArray(baseOpts, testModule);
    }

    /**
     * Helper that replays all mocks.
     */
    private void replayMocks() {
      EasyMock.replay(mMockInvocationListener, mMockITestDevice, mMockReceiver);
    }

    /**
     * Helper that verifies all mocks.
     */
    private void verifyMocks() {
      EasyMock.verify(mMockInvocationListener, mMockITestDevice, mMockReceiver);
    }

    /**
     * Test the run method with a normal input.
     * @throws DeviceNotAvailableException
     */
    public void testRunNormalInput() throws DeviceNotAvailableException {
        mTest.setDevice(mMockITestDevice);
        replayMocks();
        mTest.setTestCasePath(TEST_CASE_PATH);
        mTest.setTestConfigPath(TEST_CONFIG_PATH);
        mRunUtil.setEnvVariable(VtsMultiDeviceTest.PYTHONPATH, mPythonPath);
        mTest.setRunUtil(mRunUtil);
        mCommandResult = new CommandResult();
        mCommandResult.setStatus(CommandStatus.SUCCESS);
        EasyMock.expect(mRunUtil.runTimedCmd(VtsMultiDeviceTest.TEST_TIMEOUT, mCommand)).
            andStubReturn(mCommandResult);
        mTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /**
     * Test the run method with a normal input command failed.
     * @throws DeviceNotAvailableException
     */
    public void testRunNormalInputCommandFailed() throws DeviceNotAvailableException {
        mTest.setDevice(mMockITestDevice);
        replayMocks();
        mTest.setTestCasePath(TEST_CASE_PATH);
        mTest.setTestConfigPath(TEST_CONFIG_PATH);
        mRunUtil.setEnvVariable(VtsMultiDeviceTest.PYTHONPATH, mPythonPath);
        mTest.setRunUtil(mRunUtil);
        mCommandResult = new CommandResult();
        mCommandResult.setStatus(CommandStatus.FAILED);
        EasyMock.expect(mRunUtil.runTimedCmd(VtsMultiDeviceTest.TEST_TIMEOUT, mCommand)).
            andStubReturn(mCommandResult);
        mTest.run(mMockInvocationListener);
        verifyMocks();
    }

    /**
     * Test the run method with abnormal input data.
     */
    public void testRunAbnormalInput() throws DeviceNotAvailableException {
        mTest.setDevice(mMockITestDevice);
        replayMocks();

        try {
            mTest.run(mMockInvocationListener);
            fail("Exception didn't occur");
        } catch (IllegalArgumentException e) {
            // expected
        }
        verifyMocks();
    }

    /**
     * Test the run method without any device.
     */
    public void testRunDevice() throws DeviceNotAvailableException {
        mTest.setDevice(null);
        replayMocks();

        try {
            mTest.run(mMockInvocationListener);
            fail("Exception didn't occur");
        } catch (IllegalArgumentException e) {
            // expected
        }
        verifyMocks();
    }
}
