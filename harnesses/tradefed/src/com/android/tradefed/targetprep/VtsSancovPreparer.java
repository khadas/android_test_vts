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
 * Preparer class for sanitizer coverage.
 *
 * <p>This preparer fetches the unstripped binaries and copies them to a temporary directory whose
 * path is passed down with the test IBuildInfo object. It also prepares the device by remounting
 * the system partition for write access.
 */
@OptionClass(alias = "vts-sancov-preparer")
public class VtsSancovPreparer implements ITargetPreparer, ITargetCleaner {
    private static final long BASE_TIMEOUT = 1000 * 60 * 20;
    private static final String BUILD_INFO_ARTIFACT = "BUILD_INFO";
    private static final String SYMBOLS_ARTIFACT = "%s-symbols-%s.zip";
    private static final String SYMBOLS_FILE_NAME = "symbols.zip";
    private static final String SANCOV_FLAVOR = "_asan_coverage";
    private static final String SANCOV_RESOURCES_KEY = "sancov-resources-path-%s";

    private static final String SANCOV_CONFIGURE_SRC = "DATA/bin/vts_sancov_configure";
    private static final String SANCOV_CONFIGURE_DST = "/data/local/tmp/vts_sancov_configure";

    private File mDeviceInfoPath = null;

    /** {@inheritDoc} */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws DeviceNotAvailableException {
        String flavor = device.getBuildFlavor();
        String buildId = device.getBuildId();

        if (flavor == null || buildId == null || !flavor.contains(SANCOV_FLAVOR)) {
            CLog.i("Sancov disabled.");
            return;
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
                CLog.e("Vendor configuration required.");
                return;
            }

            // Create a temporary coverage directory
            mDeviceInfoPath = FileUtil.createTempDir(device.getSerialNumber());

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

            // Fetch the device build information file
            cmdString = String.format(artifactFetcher, buildId, flavor, BUILD_INFO_ARTIFACT,
                    mDeviceInfoPath.getAbsolutePath().toString());
            cmd = cmdString.split("\\s+");
            commandResult = runUtil.runTimedCmd(BASE_TIMEOUT, cmd);
            artifactFile = new File(mDeviceInfoPath, BUILD_INFO_ARTIFACT);
            if (commandResult == null || commandResult.getStatus() != CommandStatus.SUCCESS
                    || !artifactFile.exists()) {
                CLog.e("Could not fetch build info.");
                mDeviceInfoPath = null;
                return;
            }

            // Push the sancov flushing tool
            device.pushFile(new File(SANCOV_CONFIGURE_SRC), SANCOV_CONFIGURE_DST);
            device.executeShellCommand("setenforce 0");
            buildInfo.setFile(
                    getSancovResourceDirKey(device), mDeviceInfoPath, buildInfo.getBuildId());
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
        if (mDeviceInfoPath != null) {
            FileUtil.recursiveDelete(mDeviceInfoPath);
            device.executeShellCommand("rm -r " + SANCOV_CONFIGURE_DST);
        }
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
}
