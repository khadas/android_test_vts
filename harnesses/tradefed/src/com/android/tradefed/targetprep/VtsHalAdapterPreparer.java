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

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.util.CmdUtil;

import java.util.function.Predicate;
/**
 * Starts and stops a HAL (Hardware Abstraction Layer) adapter.
 */
@OptionClass(alias = "vts-hal-adapter-preparer")
public class VtsHalAdapterPreparer
        implements ITargetPreparer, ITargetCleaner, IMultiTargetPreparer, IAbiReceiver {
    static final int THREAD_COUNT_DEFAULT = 1;
    static final long FRAMEWORK_START_TIMEOUT = 1000 * 60 * 2; // 2 minutes.

    // The path of a sysprop to stop HIDL adapaters. Currently, there's one global flag for all
    // adapters.
    static final String ADAPTER_SYSPROP = "test.hidl.adapters.deactivated";
    // The wrapper script to start an adapter binary in the background.
    static final String SCRIPT_PATH = "/data/local/tmp/vts_adapter.sh";
    // Command to list the registered instance for the given hal@version.
    static final String LIST_HAL_CMD =
            "lshal -ti --neat | grep -E '^hwbinder' | awk '{print $2}' | grep %s";

    @Option(name = "adapter-binary-name",
            description = "Adapter binary file name (typically under /data/nativetest*/)")
    private String mAdapterBinaryName = null;

    @Option(name = "hal-package-name", description = "Target hal to adapter")
    private String mPackageName = null;

    @Option(name = "thread-count", description = "HAL adapter's thread count")
    private int mThreadCount = THREAD_COUNT_DEFAULT;

    // Application Binary Interface (ABI) info of the current test run.
    private IAbi mAbi = null;

    // CmdUtil help to verify the cmd results.
    private CmdUtil mCmdUtil = null;
    // Predicates to stop retrying cmd.
    private Predicate<String> mCheckEmpty = (String str) -> {
        return str.isEmpty();
    };
    private Predicate<String> mCheckNonEmpty = (String str) -> {
        return !str.isEmpty();
    };
    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        device.executeShellCommand(String.format("setprop %s false", ADAPTER_SYSPROP));
        String bitness =
                (mAbi != null) ? ((mAbi.getBitness() == "32") ? "" : mAbi.getBitness()) : "";
        String out = device.executeShellCommand(String.format(LIST_HAL_CMD, mPackageName));
        for (String line : out.split("\n")) {
            if (!line.isEmpty()) {
                String interfaceInstance = line.split("::", 2)[1];
                String interfaceName = interfaceInstance.split("/", 2)[0];
                String instanceName = interfaceInstance.split("/", 2)[1];
                // starts adapter
                String command = String.format("%s /data/nativetest%s/%s %s %s %d", SCRIPT_PATH,
                        bitness, mAdapterBinaryName, interfaceName, instanceName, mThreadCount);
                CLog.i("Trying to adapter for %s",
                        mPackageName + "::" + interfaceName + "/" + instanceName);
                device.executeShellCommand(command);
            }
        }

        mCmdUtil = mCmdUtil != null ? mCmdUtil : new CmdUtil();
        if (!mCmdUtil.waitCmdResultWithDelay(
                    device, String.format(LIST_HAL_CMD, mPackageName), mCheckEmpty)) {
            throw new RuntimeException("Hal adapter failed.");
        }

        device.executeShellCommand("stop");
        device.executeShellCommand("start");

        if (!device.waitForBootComplete(FRAMEWORK_START_TIMEOUT)) {
            throw new DeviceNotAvailableException("Framework failed to start.");
        }

        if (!mCmdUtil.waitCmdResultWithDelay(
                    device, "service list | grep  package", mCheckNonEmpty)) {
            throw new RuntimeException("Failed to start package service");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(IInvocationContext context)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        setUp(context.getDevices().get(0), context.getBuildInfos().get(0));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        // stops adapter
        device.executeShellCommand(String.format("setprop %s true", ADAPTER_SYSPROP));
        mCmdUtil = mCmdUtil != null ? mCmdUtil : new CmdUtil();
        if (!mCmdUtil.waitCmdResultWithDelay(
                    device, String.format(LIST_HAL_CMD, mPackageName), mCheckNonEmpty)) {
            throw new RuntimeException("Hal restore failed.");
        }

        device.executeShellCommand("stop");
        device.executeShellCommand("start");

        if (!device.waitForBootComplete(FRAMEWORK_START_TIMEOUT)) {
            throw new DeviceNotAvailableException("Framework failed to start.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(IInvocationContext context, Throwable e)
            throws DeviceNotAvailableException {
        tearDown(context.getDevices().get(0), context.getBuildInfos().get(0), e);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    @VisibleForTesting
    void setCmdUtil(CmdUtil cmdUtil) {
        mCmdUtil = cmdUtil;
    }
}
