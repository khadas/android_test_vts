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

import com.android.ddmlib.MultiLineReceiver;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.HashSet;
import java.util.Set;

/**
 * A Test that runs a vts multi device test package (part of Vendor Test Suite,
 * VTS) on given device.
 */

@OptionClass(alias = "vtsmultidevicetest")
public class VtsMultiDeviceTest implements IDeviceTest, IRemoteTest, ITestFilterReceiver,
    IRuntimeHintProvider, ITestCollector, IBuildReceiver {

    static final String PYTHONPATH = "PYTHONPATH";
    static final String VTS = "vts";
    static final float DEFAULT_TARGET_VERSION = -1;

    private ITestDevice mDevice = null;

    @Option(name = "test-timeout", description = "maximum amount of time"
        + "(im milliseconds) tests are allowed to run",
        isTimeVal = true)
    private static long TEST_TIMEOUT = 1000 * 60 * 5;

    @Option(name = "test-case-path",
        description = "The path for test case.")
    private String mTestCasePath = null;

    @Option(name = "test-config-path",
        description = "The path for test case config file.")
    private String mTestConfigPath = null;

    @Option(name = "include-filter",
        description = "The positive filter of the test names to run.")
    private Set<String> mIncludeFilters = new HashSet<>();

    @Option(name = "exclude-filter",
        description = "The negative filter of the test names to run.")
    private Set<String> mExcludeFilters = new HashSet<>();

    @Option(name = "runtime-hint", description = "The hint about the test's runtime.",
        isTimeVal = true)
    private long mRuntimeHint = 60000;  // 1 minute

    @Option(name = "collect-tests-only",
        description = "Only invoke the test binary to collect list of applicable test cases. "
                + "All test run callbacks will be triggered, but test execution will "
                + "not be actually carried out.")
    private boolean mCollectTestsOnly = false;

    // This variable is set in order to include the directory that contains the
    // python test cases. This is set before calling the method.
    // {@link #doRunTest(IRunUtil, String, String)}.
    public String mPythonPath = null;

    @Option(name = "python-binary", description = "python binary to use "
        + "(optional)")
    private String mPythonBin = null;
    private IRunUtil mRunUtil = null;
    private IBuildInfo mBuildInfo = null;
    private String mRunName = "VtsHostDrivenTest";

    /**
     * @return the mRunUtil
     */
    public IRunUtil getRunUtil() {
        return mRunUtil;
    }

    /**
     * @param mRunUtil the mRunUtil to set
     */
    public void setRunUtil(IRunUtil mRunUtil) {
        this.mRunUtil = mRunUtil;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    public void setTestCasePath(String path){
        mTestCasePath = path;
    }

    public void setTestConfigPath(String path){
        mTestConfigPath = path;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addIncludeFilter(String filter) {
        mIncludeFilters.add(cleanFilter(filter));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllIncludeFilters(Set<String> filters) {
        for (String filter : filters) {
            mIncludeFilters.add(cleanFilter(filter));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addExcludeFilter(String filter) {
        mExcludeFilters.add(cleanFilter(filter));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllExcludeFilters(Set<String> filters) {
        for (String filter : filters) {
            mExcludeFilters.add(cleanFilter(filter));
        }
    }

    /*
     * Conforms filters using a {@link com.android.ddmlib.testrunner.TestIdentifier} format
     * to be recognized by the GTest executable.
     */
    private String cleanFilter(String filter) {
        return filter.replace('#', '.');
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long getRuntimeHint() {
        return mRuntimeHint;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setCollectTestsOnly(boolean shouldCollectTest) {
        mCollectTestsOnly = shouldCollectTest;
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("deprecation")
    @Override
    public void run(ITestInvocationListener listener)
        throws IllegalArgumentException, DeviceNotAvailableException {
        if (mDevice == null) {
            throw new DeviceNotAvailableException("Device has not been set");
        }

        if (mTestCasePath == null) {
            throw new IllegalArgumentException("test-case-path is not set.");
        }

        if (mTestConfigPath == null) {
            throw new IllegalArgumentException("test-config-path is not set.");
        }

        setPythonPath();

        if (mPythonBin == null) {
            mPythonBin = getPythonBinary();
        }

        if (mRunUtil == null){
            mRunUtil = new RunUtil();
            mRunUtil.setEnvVariable(PYTHONPATH, mPythonPath);
        }
        doRunTest(listener, mRunUtil, mTestCasePath, mTestConfigPath);
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * This method prepares a command for the test and runs the python file as
     * given in the arguments.
     *
     * @param listener
     * @param runUtil
     * @param mTestCasePath
     * @param mTestConfigPath
     */

    private void doRunTest(ITestRunListener listener, IRunUtil runUtil, String mTestCasePath,
        String mTestConfigPath) throws RuntimeException {
        CLog.i("Device serial number: " + mDevice.getSerialNumber());

        if (mPythonBin == null){
            mPythonBin = getPythonBinary();
        }
        String[] baseOpts = {mPythonBin, "-m"};
        String[] testModule = {mTestCasePath, mTestConfigPath,
                               mDevice.getSerialNumber()};
        String[] cmd;
        cmd = ArrayUtil.buildArray(baseOpts, testModule);

        CommandResult commandResult = runUtil.runTimedCmd(TEST_TIMEOUT, cmd);

        if (commandResult != null && commandResult.getStatus() !=
              CommandStatus.SUCCESS) {
            CLog.e("Python process failed");
            CLog.e("Python path: %s", mPythonPath);
            CLog.e("Stderr: %s", commandResult.getStderr());
            CLog.e("Stdout: %s", commandResult.getStdout());
            throw new RuntimeException("Failed to run python unit test");
        }
        if (commandResult != null){
            CLog.i("Standard output is: %s", commandResult.getStdout());
            CLog.i("Parsing test result: %s", commandResult.getStderr());
        }

        MultiLineReceiver parser = new VtsMultiDeviceTestResultParser(listener,
            mRunName);

        if (commandResult.getStdout() != null) {
            parser.processNewLines(commandResult.getStdout().split("\n"));
        }
     }

    /**
     * This method sets the python path. It's based on the based on the
     * assumption that the environment variable $ANDROID_BUILD_TOP is set.
     */
    private void setPythonPath() {
        StringBuilder sb = new StringBuilder();
        sb.append(System.getenv(PYTHONPATH));

        // to get the path for android-vts/testcases/ which keeps the VTS python code under vts.
        if (mBuildInfo != null) {
            CompatibilityBuildHelper mBuildHelper = new CompatibilityBuildHelper(mBuildInfo);
            mBuildHelper.init(VTS, null);

            File testDir = null;
            try {
                testDir = mBuildHelper.getTestsDir();
            } catch(FileNotFoundException e) {
                /* pass */
            }
            if (testDir != null) {
                sb.append(":");
                sb.append(testDir.getAbsolutePath());
            } else if (mBuildInfo.getFile(VTS) != null) {
                sb.append(":");
                sb.append(mBuildInfo.getFile(VTS).getAbsolutePath()).append("/..");
            }
        }

        // for when one uses PythonVirtualenvPreparer.
        if (mBuildInfo.getFile(PYTHONPATH) != null) {
            sb.append(":");
            sb.append(mBuildInfo.getFile(PYTHONPATH).getAbsolutePath());
        }
        if (System.getenv("ANDROID_BUILD_TOP") != null) {
            sb.append(":");
            sb.append(System.getenv("ANDROID_BUILD_TOP")).append("/test");
        }
        mPythonPath = sb.toString();
        CLog.i("mPythonPath: %s", mPythonPath);
    }

    /**
     * This method gets the python binary
     */
    private String getPythonBinary() {
        IRunUtil runUtil = RunUtil.getDefault();
        CommandResult c = runUtil.runTimedCmd(1000, "which", "python");
        String pythonBin = c.getStdout().trim();
        if (pythonBin.length() == 0) {
            throw new RuntimeException("Could not find python binary on host "
                + "machine");
        }
        return pythonBin;
    }
 }
