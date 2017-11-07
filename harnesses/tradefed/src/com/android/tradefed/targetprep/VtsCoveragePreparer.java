/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.VtsVendorConfigFileUtil;
import java.io.File;
import java.io.IOException;
import java.util.NoSuchElementException;

/**
 * Preparer class for sanitizer and gcov coverage.
 *
 * <p>For devices instrumented with sanitizer coverage, this preparer fetches the unstripped
 * binaries and copies them to a temporary directory whose path is passed down with the test
 * IBuildInfo object. For gcov-instrumented devices, the zip containing gcno files is retrieved
 * along with the build info artifact.
 */
@OptionClass(alias = "vts-coverage-preparer")
public class VtsCoveragePreparer implements ITargetPreparer, ITargetCleaner {
    private static final long BASE_TIMEOUT = 1000 * 60 * 20; // timeout for fetching artifacts
    private static final String BUILD_INFO_ARTIFACT = "BUILD_INFO"; // name of build info artifact
    private static final String GCOV_PROPERTY = "ro.vts.coverage"; // indicates gcov when val is 1
    private static final String GCOV_ARTIFACT = "%s-coverage-%s.zip"; // gcov coverage artifact
    private static final String GCOV_FILE_NAME = "gcov.zip"; // gcov zip file to pass to VTS

    private static final String SELINUX_DISABLED = "Disabled"; // selinux disabled
    private static final String SELINUX_ENFORCING = "Enforcing"; // selinux enforcing mode
    private static final String SELINUX_PERMISSIVE = "Permissive"; // selinux permissive mode

    private static final String SYMBOLS_ARTIFACT = "%s-symbols-%s.zip"; // symbolized binary zip
    private static final String SYMBOLS_FILE_NAME = "symbols.zip"; // sancov zip to pass to VTS
    private static final String SANCOV_FLAVOR = "_asan_coverage"; // sancov device build flavor

    // Build key for gcov resources
    private static final String GCOV_RESOURCES_KEY = "gcov-resources-path-%s";

    // Buid key for sancov resources
    private static final String SANCOV_RESOURCES_KEY = "sancov-resources-path-%s";

    // Relative path to coverage configure binary in VTS package
    private static final String COVERAGE_CONFIGURE_SRC = "DATA/bin/vts_coverage_configure";

    // Target path for coverage configure binary, will be removed in teardown
    private static final String COVERAGE_CONFIGURE_DST = "/data/local/tmp/vts_coverage_configure";

    private File mDeviceInfoPath = null; // host path where gcov device artifacts are stored
    private String mEnforcingState = null; // start state for selinux enforcement

    /** {@inheritDoc} */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws DeviceNotAvailableException {
        String flavor = device.getBuildFlavor();
        String buildId = device.getBuildId();

        if (buildId == null) {
            CLog.w("Missing build ID. Coverage disabled.");
            return;
        }

        boolean sancovEnabled = (flavor != null) && flavor.contains(SANCOV_FLAVOR);

        String coverageProperty = device.getProperty(GCOV_PROPERTY);
        boolean gcovEnabled = (coverageProperty != null) && coverageProperty.equals("1");

        if (!sancovEnabled && !gcovEnabled) {
            return;
        }
        if (sancovEnabled) {
            CLog.i("Sanitizer coverage processing enabled.");
        }
        if (gcovEnabled) {
            CLog.i("Gcov coverage processing enabled.");
        }

        IRunUtil runUtil = new RunUtil();

        try {
            // Load the vendor configuration
            String artifactFetcher = null;
            VtsVendorConfigFileUtil configFileUtil = new VtsVendorConfigFileUtil();
            if (configFileUtil.LoadVendorConfig(buildInfo)) {
                artifactFetcher = configFileUtil.GetVendorConfigVariable("build_artifact_fetcher");
            }
            if (artifactFetcher == null) {
                CLog.e("Vendor configuration with build_artifact_fetcher required.");
                return;
            }

            // Create a temporary coverage directory
            mDeviceInfoPath = FileUtil.createTempDir(device.getSerialNumber());

            if (sancovEnabled) {
                // Fetch the symbolized binaries
                String artifactName = String.format(
                        SYMBOLS_ARTIFACT, flavor.substring(0, flavor.lastIndexOf("-")), buildId);
                File artifactFile = new File(mDeviceInfoPath, SYMBOLS_FILE_NAME);

                String cmdString = String.format(artifactFetcher, buildId, flavor, artifactName,
                        artifactFile.getAbsolutePath().toString());
                String[] cmd = cmdString.split("\\s+");
                CommandResult commandResult = runUtil.runTimedCmd(BASE_TIMEOUT, cmd);
                if (commandResult == null || commandResult.getStatus() != CommandStatus.SUCCESS
                        || !artifactFile.exists()) {
                    CLog.e("Could not fetch unstripped binaries.");
                    return;
                }
            }

            if (gcovEnabled) {
                // Fetch the symbolized binaries
                String artifactName = String.format(
                        GCOV_ARTIFACT, flavor.substring(0, flavor.lastIndexOf("-")), buildId);
                File artifactFile = new File(mDeviceInfoPath, GCOV_FILE_NAME);

                String cmdString = String.format(artifactFetcher, buildId, flavor, artifactName,
                        artifactFile.getAbsolutePath().toString());
                String[] cmd = cmdString.split("\\s+");
                CommandResult commandResult = runUtil.runTimedCmd(BASE_TIMEOUT, cmd);
                if (commandResult == null || commandResult.getStatus() != CommandStatus.SUCCESS
                        || !artifactFile.exists()) {
                    CLog.e("Could not fetch gcov build artifacts.");
                    return;
                }
            }

            // Fetch the device build information file
            String cmdString = String.format(artifactFetcher, buildId, flavor, BUILD_INFO_ARTIFACT,
                    mDeviceInfoPath.getAbsolutePath().toString());
            String[] cmd = cmdString.split("\\s+");
            CommandResult commandResult = runUtil.runTimedCmd(BASE_TIMEOUT, cmd);
            File artifactFile = new File(mDeviceInfoPath, BUILD_INFO_ARTIFACT);
            if (commandResult == null || commandResult.getStatus() != CommandStatus.SUCCESS
                    || !artifactFile.exists()) {
                CLog.e("Could not fetch build info.");
                mDeviceInfoPath = null;
                return;
            }

            // Push the sancov flushing tool
            device.pushFile(new File(COVERAGE_CONFIGURE_SRC), COVERAGE_CONFIGURE_DST);
            device.executeShellCommand("rm -rf /data/misc/trace/*");
            mEnforcingState = device.executeShellCommand("getenforce");
            if (!mEnforcingState.equals(SELINUX_DISABLED)
                    && !mEnforcingState.equals(SELINUX_PERMISSIVE)) {
                device.executeShellCommand("setenforce " + SELINUX_PERMISSIVE);
            }

            if (sancovEnabled) {
                buildInfo.setFile(
                        getSancovResourceDirKey(device), mDeviceInfoPath, buildInfo.getBuildId());
            }

            if (gcovEnabled) {
                buildInfo.setFile(
                        getGcovResourceDirKey(device), mDeviceInfoPath, buildInfo.getBuildId());
            }
        } catch (IOException | NoSuchElementException e) {
            CLog.e("Could not set up sanitizer coverage: " + e.toString());
            mDeviceInfoPath = null;
        }
    }

    /** {@inheritDoc} */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        // Clear the temporary directories.
        if (!mEnforcingState.equals(SELINUX_DISABLED)) {
            device.executeShellCommand("setenforce " + mEnforcingState);
        }
        if (mDeviceInfoPath != null) {
            FileUtil.recursiveDelete(mDeviceInfoPath);
            device.executeShellCommand("rm -r " + COVERAGE_CONFIGURE_DST);
        }
        device.executeShellCommand("rm -rf /data/misc/trace/*");
    }

    /**
     * Get the key of the symbolized binary directory for the specified device.
     *
     * @param device the target device whose sancov resources directory to get.
     * @return the (String) key name of the device's sancov resources directory.
     */
    public static String getSancovResourceDirKey(ITestDevice device) {
        return String.format(SANCOV_RESOURCES_KEY, device.getSerialNumber());
    }

    /**
     * Get the key of the gcov resources for the specified device.
     *
     * @param device the target device whose sancov resources directory to get.
     * @return the (String) key name of the device's gcov resources directory.
     */
    public static String getGcovResourceDirKey(ITestDevice device) {
        return String.format(GCOV_RESOURCES_KEY, device.getSerialNumber());
    }
}
