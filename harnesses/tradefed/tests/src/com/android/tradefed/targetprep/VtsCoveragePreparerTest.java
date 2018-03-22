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

import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.*;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.InjectMocks;
import org.mockito.Mockito;
/**
 * Unit tests for {@link VtsCoveragePreparer},
 */
@RunWith(JUnit4.class)
public final class VtsCoveragePreparerTest {
    private String BUILD_INFO_ARTIFACT = VtsCoveragePreparer.BUILD_INFO_ARTIFACT;
    private String GCOV_PROPERTY = VtsCoveragePreparer.GCOV_PROPERTY;
    private String GCOV_ARTIFACT = VtsCoveragePreparer.GCOV_ARTIFACT;
    private String GCOV_FILE_NAME = VtsCoveragePreparer.GCOV_FILE_NAME;
    private String SYMBOLS_ARTIFACT = VtsCoveragePreparer.SYMBOLS_ARTIFACT;
    private String SYMBOLS_FILE_NAME = VtsCoveragePreparer.SYMBOLS_FILE_NAME;
    private String SANCOV_FLAVOR = VtsCoveragePreparer.SANCOV_FLAVOR;
    private String COVERAGE_REPORT_PATH = VtsCoveragePreparer.COVERAGE_REPORT_PATH;

    // Path to store coverage test files.
    private static final String TEST_COVERAGE_RESOURCE_PATH = "/tmp/test-coverage/";

    private class TestCoveragePreparer extends VtsCoveragePreparer {
        @Override
        File createTempDir(ITestDevice device) {
            return new File(TEST_COVERAGE_RESOURCE_PATH);
        }

        @Override
        String getArtifactFetcher(IBuildInfo buildInfo) {
            return "fetcher --bid %s --target %s %s %s";
        }
    }
    private File mDeviceInfoPath = null;

    @Mock private IBuildInfo mBuildInfo;
    @Mock private ITestDevice mDevice;
    @Mock private IRunUtil mRunUtil;
    @InjectMocks private TestCoveragePreparer mPreparer = new TestCoveragePreparer();

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        doReturn("build_id").when(mDevice).getBuildId();
        doReturn("1234").when(mDevice).getSerialNumber();
        doReturn("enforcing").when(mDevice).executeShellCommand("getenforce");
        doReturn("build_id").when(mBuildInfo).getBuildId();
        mDeviceInfoPath = new File(TEST_COVERAGE_RESOURCE_PATH);
        mDeviceInfoPath.mkdirs();
        CommandResult commandResult = new CommandResult();
        commandResult.setStatus(CommandStatus.SUCCESS);
        doReturn(commandResult).when(mRunUtil).runTimedCmd(anyLong(), anyObject());
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.recursiveDelete(mDeviceInfoPath);
    }

    @Test
    public void testOnSetUpCoverageDisabled() throws Exception {
        doReturn("UnknowFlaver").when(mDevice).getBuildFlavor();
        doReturn("None").when(mDevice).getProperty(GCOV_PROPERTY);

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, never()).setFile(anyObject(), anyObject(), anyObject());
    }

    @Test
    public void testOnSetUpSancovEnabled() throws Exception {
        doReturn("walleye_asan_coverage-userdebug").when(mDevice).getBuildFlavor();
        createTestFile(SYMBOLS_FILE_NAME);
        createTestFile(BUILD_INFO_ARTIFACT);

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, times(1))
                .setFile(eq(VtsCoveragePreparer.getSancovResourceDirKey(mDevice)),
                        eq(mDeviceInfoPath), eq("build_id"));
    }

    @Test
    public void testOnSetUpGcovEnabled() throws Exception {
        doReturn("walleye_coverage-userdebug").when(mDevice).getBuildFlavor();
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);
        createTestFile(GCOV_FILE_NAME);
        createTestFile(BUILD_INFO_ARTIFACT);
        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, times(1))
                .setFile(eq(VtsCoveragePreparer.getGcovResourceDirKey(mDevice)),
                        eq(mDeviceInfoPath), eq("build_id"));
    }

    @Test
    public void testOnSetUpLocalArtifectsNormal() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("use-local-artifects", "true");
        setter.setOptionValue("local-coverage-resource-path", "/tmp/test-coverage/");
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);
        createTestFile(GCOV_FILE_NAME);
        createTestFile(BUILD_INFO_ARTIFACT);

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, times(1))
                .setFile(eq(VtsCoveragePreparer.getGcovResourceDirKey(mDevice)),
                        eq(mDeviceInfoPath), eq("build_id"));
    }

    @Test
    public void testOnSetUpLocalArtifectsNotExists() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("use-local-artifects", "true");
        setter.setOptionValue("local-coverage-resource-path", TEST_COVERAGE_RESOURCE_PATH);
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);

        mPreparer.setUp(mDevice, mBuildInfo);
        verify(mBuildInfo, never()).setFile(anyObject(), anyObject(), anyObject());
    }

    @Test
    public void testOnSetUpOutputCoverageReport() throws Exception {
        OptionSetter setter = new OptionSetter(mPreparer);
        setter.setOptionValue("coverage-report-dir", TEST_COVERAGE_RESOURCE_PATH);
        doReturn("walleye_coverage-userdebug").when(mDevice).getBuildFlavor();
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);
        createTestFile(GCOV_FILE_NAME);
        createTestFile(BUILD_INFO_ARTIFACT);

        mPreparer.setUp(mDevice, mBuildInfo);
        String currentDate =
                new SimpleDateFormat("yyyy-MM-dd").format(new Date(System.currentTimeMillis()));
        verify(mBuildInfo, times(1))
                .addBuildAttribute(
                        eq(COVERAGE_REPORT_PATH), eq(TEST_COVERAGE_RESOURCE_PATH + currentDate));
    }

    @Test
    public void testOnTearDown() throws Exception {
        doReturn("walleye_coverage-userdebug").when(mDevice).getBuildFlavor();
        doReturn("1").when(mDevice).getProperty(GCOV_PROPERTY);
        File artifectsFile = createTestFile(GCOV_FILE_NAME);
        File buildInfoFile = createTestFile(BUILD_INFO_ARTIFACT);
        mPreparer.setUp(mDevice, mBuildInfo);
        mPreparer.tearDown(mDevice, mBuildInfo, null);
        verify(mDevice, times(1)).executeShellCommand("setenforce enforcing");
        assertFalse(artifectsFile.exists());
        assertFalse(buildInfoFile.exists());
    }

    /**
     * Helper method to create a test file under mDeviceInfoPath.
     *
     * @fileName test file name.
     * @return created test file.
     */
    private File createTestFile(String fileName) throws IOException {
        File testFile = new File(mDeviceInfoPath, fileName);
        testFile.createNewFile();
        return testFile;
    }
}
