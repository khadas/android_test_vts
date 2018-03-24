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

package com.android.tradefed.targetprep;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.*;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.util.CmdUtil;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.InjectMocks;
import org.mockito.Mockito;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Predicate;
/**
 * Unit tests for {@link VtsHalAdapterPreparer},
 */
@RunWith(JUnit4.class)
public final class VtsHalAdapterPreparerTest {
    private int THREAD_COUNT_DEFAULT = VtsHalAdapterPreparer.THREAD_COUNT_DEFAULT;
    private long FRAMEWORK_START_TIMEOUT = VtsHalAdapterPreparer.FRAMEWORK_START_TIMEOUT;
    private String SCRIPT_PATH = VtsHalAdapterPreparer.SCRIPT_PATH;
    private String LIST_HAL_CMD = VtsHalAdapterPreparer.LIST_HAL_CMD;

    private String TEST_HAL_ADAPTER_BINARY = "android.hardware.foo@1.0-adapter";
    private String TEST_HAL_PACKAGE = "android.hardware.foo@1.1";

    private class TestCmdUtil extends CmdUtil {
        public boolean mCmdSuccess = true;
        @Override
        public boolean waitCmdResultWithDelay(ITestDevice device, String cmd,
                Predicate<String> predicate) throws DeviceNotAvailableException {
            return mCmdSuccess;
        }
    }

    private TestCmdUtil mCmdUtil = new TestCmdUtil();

    @Mock private IBuildInfo mBuildInfo;
    @Mock private ITestDevice mDevice;
    @Mock private IAbi mAbi;
    @InjectMocks private VtsHalAdapterPreparer mPreparer;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mPreparer.setCmdUtil(mCmdUtil);
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("adapter-binary-name", TEST_HAL_ADAPTER_BINARY);
        setter.setOptionValue("hal-package-name", TEST_HAL_PACKAGE);
        doReturn(true).when(mDevice).waitForBootComplete(FRAMEWORK_START_TIMEOUT);
        doReturn("64").when(mAbi).getBitness();
    }

    @Test
    public void testOnSetUpAdapterSingleInstance() throws Exception {
        String output = "android.hardware.foo@1.1::IFoo/default";
        doReturn(output).when(mDevice).executeShellCommand(
                String.format(LIST_HAL_CMD, TEST_HAL_PACKAGE));

        mPreparer.setUp(mDevice, mBuildInfo);
        String adapterCmd = String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFoo", "default", THREAD_COUNT_DEFAULT);
        verify(mDevice, times(1)).executeShellCommand(eq(adapterCmd));
    }

    @Test
    public void testOnSetUpAdapterMultipleInstance() throws Exception {
        String output = "android.hardware.foo@1.1::IFoo/default\n"
                + "android.hardware.foo@1.1::IFoo/test\n"
                + "android.hardware.foo@1.1::IFooSecond/default\n"
                + "android.hardware.foo@1.1::IFooSecond/slot/1\n";
        doReturn(output).when(mDevice).executeShellCommand(
                String.format(LIST_HAL_CMD, TEST_HAL_PACKAGE));

        mPreparer.setUp(mDevice, mBuildInfo);

        List<String> adapterCmds = new ArrayList<String>();
        adapterCmds.add(String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFoo", "default", THREAD_COUNT_DEFAULT));
        adapterCmds.add(String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFoo", "test", THREAD_COUNT_DEFAULT));
        adapterCmds.add(String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFooSecond", "default", THREAD_COUNT_DEFAULT));
        adapterCmds.add(String.format("%s /data/nativetest64/%s %s %s %d", SCRIPT_PATH,
                TEST_HAL_ADAPTER_BINARY, "IFooSecond", "slot/1", THREAD_COUNT_DEFAULT));

        for (String cmd : adapterCmds) {
            verify(mDevice, times(1)).executeShellCommand(eq(cmd));
        }
    }

    @Test
    public void testOnSetUpAdapterFailed() throws Exception {
        String output = "android.hardware.foo@1.1::IFoo/default";
        doReturn(output).when(mDevice).executeShellCommand(
                String.format(LIST_HAL_CMD, TEST_HAL_PACKAGE));
        mCmdUtil.mCmdSuccess = false;
        try {
            mPreparer.setUp(mDevice, mBuildInfo);
        } catch (RuntimeException e) {
            assertEquals("Hal adapter failed.", e.getMessage());
            return;
        }
        fail();
    }

    @Test
    public void testOnTearDownRestoreFailed() throws Exception {
        mCmdUtil.mCmdSuccess = false;
        try {
            mPreparer.tearDown(mDevice, mBuildInfo, null);
        } catch (RuntimeException e) {
            assertEquals("Hal restore failed.", e.getMessage());
            return;
        }
        fail();
    }
}
