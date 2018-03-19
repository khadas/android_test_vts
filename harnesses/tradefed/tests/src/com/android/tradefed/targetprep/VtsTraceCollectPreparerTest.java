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

import com.android.compatibility.common.tradefed.build.VtsCompatibilityInvocationHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.NoSuchElementException;
import junit.framework.AssertionFailedError;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit tests for {@link VtsTraceCollectPreparer},
 */
@RunWith(JUnit4.class)
public final class VtsTraceCollectPreparerTest {
    private String SELINUX_DISABLED = VtsTraceCollectPreparer.SELINUX_DISABLED;
    private String SELINUX_PERMISSIVE = VtsTraceCollectPreparer.SELINUX_PERMISSIVE;
    private String VTS_LIB_DIR_32 = VtsTraceCollectPreparer.VTS_LIB_DIR_32;
    private String VTS_LIB_DIR_64 = VtsTraceCollectPreparer.VTS_LIB_DIR_64;
    private String VTS_BINARY_DIR = VtsTraceCollectPreparer.VTS_BINARY_DIR;
    private String VTS_TMP_LIB_DIR_32 = VtsTraceCollectPreparer.VTS_TMP_LIB_DIR_32;
    private String VTS_TMP_LIB_DIR_64 = VtsTraceCollectPreparer.VTS_TMP_LIB_DIR_64;
    private String VTS_TMP_DIR = VtsTraceCollectPreparer.VTS_TMP_DIR;
    private String PROFILING_CONFIGURE_BINARY = VtsTraceCollectPreparer.PROFILING_CONFIGURE_BINARY;
    private String TRACE_PATH = VtsTraceCollectPreparer.TRACE_PATH;

    private static final String TEST_VTS_PROFILER = "test-vts.profiler.so";
    private static final String TEST_VTS_LIB = "libvts-for-test.so";
    private static final String TEST_TRACE_DIR = "/tmp/test-trace-dir/";
    private static final String mDate =
            new SimpleDateFormat("yyyy-MM-dd").format(new Date(System.currentTimeMillis()));

    private VtsTraceCollectPreparer mPreparer;
    private IBuildInfo mMockBuildInfo;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() throws Exception {
        mMockDevice = EasyMock.createNiceMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createNiceMock(IBuildInfo.class);
        mPreparer = new VtsTraceCollectPreparer();
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("local-trace-dir", TEST_TRACE_DIR);
    }

    @Test
    public void testOnSetUp() throws Exception {
        // Create test profiler and profiling related files.
        VtsCompatibilityInvocationHelper invocationHelper = new VtsCompatibilityInvocationHelper();
        File testDir = invocationHelper.getTestsDir();
        // Files that should be pushed.
        File testProfilerlib32 = new File(testDir, VTS_LIB_DIR_32 + TEST_VTS_PROFILER);
        File testProfilerlib64 = new File(testDir, VTS_LIB_DIR_64 + TEST_VTS_PROFILER);
        File testVtslib32 = new File(testDir, VTS_LIB_DIR_32 + TEST_VTS_LIB);
        File testVtslib64 = new File(testDir, VTS_LIB_DIR_64 + TEST_VTS_LIB);
        // Files that should not be pushed.
        File testUnrelatedlib32 = new File(testDir, VTS_LIB_DIR_32 + "somelib.so");
        File testUnrelatedlib64 = new File(testDir, VTS_LIB_DIR_64 + "somelib.so");
        testProfilerlib32.createNewFile();
        testProfilerlib64.createNewFile();
        testVtslib32.createNewFile();
        testVtslib64.createNewFile();
        testUnrelatedlib32.createNewFile();
        testUnrelatedlib64.createNewFile();

        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testProfilerlib32),
                                EasyMock.eq(VTS_TMP_LIB_DIR_32 + TEST_VTS_PROFILER)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testProfilerlib64),
                                EasyMock.eq(VTS_TMP_LIB_DIR_64 + TEST_VTS_PROFILER)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testVtslib32),
                                EasyMock.eq(VTS_TMP_LIB_DIR_32 + TEST_VTS_LIB)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testVtslib64),
                                EasyMock.eq(VTS_TMP_LIB_DIR_64 + TEST_VTS_LIB)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testUnrelatedlib32),
                                EasyMock.eq(VTS_TMP_LIB_DIR_32 + "somelib.so")))
                .andThrow(new AssertionFailedError())
                .anyTimes();
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(testUnrelatedlib64),
                                EasyMock.eq(VTS_TMP_LIB_DIR_64 + "somelib.so")))
                .andThrow(new AssertionFailedError())
                .anyTimes();
        EasyMock.expect(mMockDevice.pushFile(EasyMock.eq(new File(testDir,
                                                     VTS_BINARY_DIR + PROFILING_CONFIGURE_BINARY)),
                                EasyMock.eq(VTS_TMP_DIR + PROFILING_CONFIGURE_BINARY)))
                .andReturn(true)
                .times(1);
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.eq("getenforce")))
                .andReturn("")
                .times(1);
        EasyMock.expect(mMockDevice.executeShellCommand(
                                EasyMock.eq("setenforce " + SELINUX_PERMISSIVE)))
                .andReturn("")
                .times(1);
        EasyMock.replay(mMockDevice);

        // Configure the trace directory path.
        File traceDir = new File(TEST_TRACE_DIR + mDate);
        mMockBuildInfo.addBuildAttribute(TRACE_PATH, traceDir.getAbsolutePath());
        EasyMock.expectLastCall().times(1);
        EasyMock.replay(mMockBuildInfo);

        // Run setUp.
        mPreparer.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockBuildInfo);
        EasyMock.verify(mMockDevice);
        // Cleanup test files.
        testProfilerlib32.delete();
        testProfilerlib64.delete();
        testVtslib32.delete();
        testVtslib64.delete();
        testUnrelatedlib32.delete();
        testUnrelatedlib64.delete();
    }

    @Test
    public void testOnSetUpPushFileException() throws Exception {
        EasyMock.expect(mMockDevice.pushFile(EasyMock.anyObject(), EasyMock.anyObject()))
                .andThrow(new NoSuchElementException("file not found."));
        EasyMock.replay(mMockDevice);
        try {
            mPreparer.setUp(mMockDevice, mMockBuildInfo);
            EasyMock.verify(mMockDevice);
        } catch (RuntimeException e) {
            // Expected.
            return;
        }
        fail();
    }

    @Test
    public void testOnTearDown() throws Exception {
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.eq("getenforce")))
                .andReturn(SELINUX_PERMISSIVE);
        EasyMock.expect(mMockDevice.executeShellCommand(
                                EasyMock.eq("setenforce " + SELINUX_PERMISSIVE)))
                .andReturn("")
                .times(1);
        EasyMock.replay(mMockDevice);
        mPreparer.setUp(mMockDevice, mMockBuildInfo);
        mPreparer.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice);
    }
}
