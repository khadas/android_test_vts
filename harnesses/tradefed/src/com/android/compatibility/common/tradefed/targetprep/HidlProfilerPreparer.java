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

package com.android.compatibility.common.tradefed.targetprep;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.Log;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;

/**
 * A {@link HidlProfilerPreparer} that attempts to enable and disable HIDL profiling on a target device.
 * <p />
 * This is used when one wants to do such setup and cleanup operations in Java instead of the
 * VTS Python runner, Python test template, or Python test case.
 */
@OptionClass(alias = "push-file")
public class HidlProfilerPreparer implements ITargetCleaner, IAbiReceiver {
    private static final String LOG_TAG = "HidlProfilerPreparer";

    private static final String TARGET_PROFILING_TRACE_PATH = "/data/local/tmp/";
    private static final String TARGET_PROFILING_LIBRARY_PATH = "/data/local/tmp/<bitness>";

    @Option(name="target-profiling-trace-path", description=
            "The target-side path to store the profiling trace file(s).")
    private String mTargetProfilingTracePath = TARGET_PROFILING_TRACE_PATH;

    @Option(name="target-profiling-library-path", description=
            "The target-side path to store the profiling trace file(s). " +
            "Use <bitness> to auto fill in 32 or 64 depending on the target device bitness.")
    private String mTargetProfilingLibraryPath = TARGET_PROFILING_LIBRARY_PATH;

    @Option(name="copy-generated-trace-files", description=
        "Whether to copy the generated trace files to a host-side, " +
        "designated destination dir")
    private boolean mCopyGeneratedTraceFiles = false;

    private IAbi mAbi = null;

    /**
     * Set mTargetProfilingTracePath.  Exposed for testing.
     */
    void setTargetProfilingTracePath(String path) {
        mTargetProfilingTracePath = path;
    }

    /**
     * Set mTargetProfilingLibraryPath.  Exposed for testing.
     */
    void setTargetProfilingLibraryPath(String path) {
      mTargetProfilingLibraryPath = path;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setAbi(IAbi abi){
        mAbi = abi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError, BuildError,
            DeviceNotAvailableException {
        // Cleanup any existing traces
        Log.d(LOG_TAG, String.format("Deleting any existing target-side trace files in %s.",
              mTargetProfilingTracePath));
        device.executeShellCommand(
                String.format("rm %s/*.vts.trace", mTargetProfilingTracePath));

        Log.d(LOG_TAG, String.format("Starting the HIDL profiling (bitness: %s).",
                                     mAbi.getBitness()));
        mTargetProfilingLibraryPath = mTargetProfilingLibraryPath.replace(
                "<bitness>", mAbi.getBitness());
        Log.d(LOG_TAG, String.format("Target Profiling Library Path: %s",
                                     mTargetProfilingLibraryPath));
        device.executeShellCommand(
                String.format("setprop hal.instrumentation.lib.path %s",
                        mTargetProfilingLibraryPath));
        device.executeShellCommand(
                "setprop hal.instrumentation.enable true");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
      Log.d(LOG_TAG, "Stopping the HIDL Profiling.");
      // Disables VTS Profiling
      device.executeShellCommand("setprop hal.instrumentation.lib.path \"\"");
      device.executeShellCommand("setprop hal.instrumentation.enable false");

      // Gets trace files from the target.
      if (!mTargetProfilingTracePath.endsWith("/")) {
        mTargetProfilingTracePath += "/";
      }
      String trace_file_list_string = device.executeShellCommand(
          String.format("ls %s*.vts.trace", mTargetProfilingTracePath));
      Log.d(LOG_TAG, String.format("Generated trace files: %s",
                                   trace_file_list_string));

      if (mCopyGeneratedTraceFiles) {
          // TODO(yim): adb pull from the target to a host-side directory (provided by a config
          // stored in a private repository).
      }
    }
}
