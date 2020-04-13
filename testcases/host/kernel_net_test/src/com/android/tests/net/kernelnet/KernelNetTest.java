/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.tests.net.kernelnet;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import java.util.concurrent.TimeUnit;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/* Host test class to run android kernel unit test. */
@RunWith(DeviceJUnit4ClassRunner.class)
public class KernelNetTest extends BaseHostJUnit4Test {
    private String mTargetBinPath;
    private static final String DEVICE_BIN_ROOT = "/data/local/tmp/kernel_net_tests";
    /** The time in ms to wait for a command to complete. */
    private static final long CMD_TIMEOUT = 5 * 60 * 1000L;
    private static final String KERNEL_NET_TESTS = "kernel_net_tests";

    /**
     * Android kernel unit test.
     * @throws DeviceNotAvailableException
     */
    @Test
    public void testKernelNetworking() throws DeviceNotAvailableException {
        // The actual test binary file might be in sub-dictionary under DEVICE_BIN_ROOT.
        mTargetBinPath = findRunKernelNetPath(DEVICE_BIN_ROOT);
        CLog.d(String.format("mTargetBinPath: %s", mTargetBinPath));
        Assert.assertNotEquals(
                String.format("No kernel_net_tests binary found in %s", DEVICE_BIN_ROOT),
                mTargetBinPath, null);

        // Execute the test binary.
        CommandResult result = getDevice().executeShellV2Command(
                mTargetBinPath, CMD_TIMEOUT, TimeUnit.MILLISECONDS);

        Assert.assertTrue(
                String.format("kernel_net_tests binary returned non-zero exit code: %s, stdout: %s",
                        result.getStderr(), result.getStdout()),
                CommandStatus.SUCCESS.equals(result.getStatus()));
    }
    /**
     * Find kernel_net_test file path under root folder.
     * Assume there is one kernel_net_test binary valid.
     * @param root The root folder to begin searching for kernel_net_test
     * @throws DeviceNotAvailableException
     */
    private String findRunKernelNetPath(String root) throws DeviceNotAvailableException {
        String mFoundPath;
        if (getDevice().isDirectory(root)) {
            // recursively find in all subdirectories
            for (String child : getDevice().getChildren(root)) {
                mFoundPath = findRunKernelNetPath(root + "/" + child);
                if (mFoundPath != null) {
                    return mFoundPath;
                }
            }
        } else if (root.endsWith("/" + KERNEL_NET_TESTS)) {
            // It is a file and check the filename.
            return root;
        }
        return null;
    }
}
