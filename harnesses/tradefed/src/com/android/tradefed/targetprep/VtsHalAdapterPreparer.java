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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;

import java.util.concurrent.TimeUnit;

/**
 * Starts and stops a HAL (Hardware Abstraction Layer) adapter.
 */
@OptionClass(alias = "vts-hal-adapter-preparer")
public class VtsHalAdapterPreparer
        implements ITargetPreparer, ITargetCleaner, IMultiTargetPreparer, IAbiReceiver {
    private static final int THREAD_COUNT_DEFAULT = 1;
    private static final String SERVICE_NAME_DEFAULT = "default";
    // The path of a sysprop to stop HIDL adapaters. Currently, there's one global flag for all
    // adapters.
    private static final String ADAPTER_SYSPROP = "test.hidl.adapters.deactivated";
    // The wrapper script to start an adapter binary in the background.
    private static final String SCRIPT_PATH = "/data/local/tmp/vts_adapter.sh";

    @Option(name = "adapter-binary-name",
            description = "Adapter binary file name (typically under /data/nativetest*/)")
    private String mAdapterBinaryName = null;

    @Option(name = "interface-name", description = "Adapter's main interface name")
    private String mInterfaceName = null;

    @Option(name = "service-name", description = "Instance service name of a HAL adapter to create")
    private String mServiceName = SERVICE_NAME_DEFAULT;

    @Option(name = "thread-count", description = "HAL adapter's thread count")
    private int mThreadCount = THREAD_COUNT_DEFAULT;

    // Application Binary Interface (ABI) info of the current test run.
    private IAbi mAbi = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        CollectingOutputReceiver out = new CollectingOutputReceiver();
        device.executeShellCommand(String.format("lshal | grep %s", mInterfaceName), out);
        CLog.i("setUp: lshal (entry):\n%s", out.getOutput());

        device.executeShellCommand(String.format("setprop %s false", ADAPTER_SYSPROP));

        // starts adapter
        String bitness =
                (mAbi != null) ? ((mAbi.getBitness() == "32") ? "" : mAbi.getBitness()) : "";
        String command = String.format(". %s /data/nativetest%s/%s %s %s %d", SCRIPT_PATH, bitness,
                mAdapterBinaryName, mInterfaceName, mServiceName, mThreadCount);
        CLog.i("Command: %s", command);
        out = new CollectingOutputReceiver();
        device.executeShellCommand(command, out);
        CLog.i("Command output:\n%s", out.getOutput());

        try {
            TimeUnit.SECONDS.sleep(3);
        } catch (InterruptedException ex) { /* pass */
        }

        device.executeShellCommand("stop");
        device.executeShellCommand("start");

        out = new CollectingOutputReceiver();
        device.executeShellCommand(String.format("lshal | grep %s", mInterfaceName), out);
        CLog.i("setUp: lshal (exit):\n%s", out.getOutput());
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
        CollectingOutputReceiver out = new CollectingOutputReceiver();
        device.executeShellCommand(String.format("lshal | grep %s", mInterfaceName), out);
        CLog.i("tearDown: lshal (entry):\n%s", out.getOutput());

        // stops adapter
        device.executeShellCommand(String.format("setprop %s true", ADAPTER_SYSPROP));
        try {
            TimeUnit.SECONDS.sleep(3);
        } catch (InterruptedException ex) { /* pass */
        }

        device.executeShellCommand("stop");
        device.executeShellCommand("start");

        out = new CollectingOutputReceiver();
        device.executeShellCommand(String.format("lshal | grep %s", mInterfaceName), out);
        CLog.i("tearDown: lshal (exit):\n%s", out.getOutput());
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
}
