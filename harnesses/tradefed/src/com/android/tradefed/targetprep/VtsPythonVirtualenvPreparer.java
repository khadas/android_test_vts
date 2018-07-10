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

package com.android.tradefed.targetprep;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.EnvUtil;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.VtsFileUtil;
import com.android.tradefed.util.VtsPythonRunnerHelper;
import com.android.tradefed.util.VtsVendorConfigFileUtil;

import java.io.File;
import java.io.IOException;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.Arrays;
import java.util.Collection;
import java.util.NoSuchElementException;
import java.util.TreeSet;

/**
 * Sets up a Python virtualenv on the host and installs packages. To activate it, the working
 * directory is changed to the root of the virtualenv.
 *
 * This's a fork of PythonVirtualenvPreparer and is forked in order to simplify the change
 * deployment process and reduce the deployment time, which are critical for VTS services.
 * That means changes here will be upstreamed gradually.
 */
@OptionClass(alias = "python-venv")
public class VtsPythonVirtualenvPreparer implements IMultiTargetPreparer {
    private static final String LOCAL_PYPI_PATH_ENV_VAR_NAME = "VTS_PYPI_PATH";
    private static final String LOCAL_PYPI_PATH_KEY = "pypi_packages_path";
    private static final int BASE_TIMEOUT = 1000 * 60;
    public static final String VIRTUAL_ENV_V3 = "VIRTUAL_ENV_V3";
    public static final String VIRTUAL_ENV = "VIRTUAL_ENV";

    @Option(name = "venv-dir", description = "path of an existing virtualenv to use")
    protected File mVenvDir = null;

    @Option(name = "requirements-file", description = "pip-formatted requirements file")
    private File mRequirementsFile = null;

    @Option(name = "script-file", description = "scripts which need to be executed in advance")
    private Collection<String> mScriptFiles = new TreeSet<>();

    @Option(name = "dep-module", description = "modules which need to be installed by pip")
    protected Collection<String> mDepModules = new TreeSet<>();

    @Option(name = "no-dep-module", description = "modules which should not be installed by pip")
    private Collection<String> mNoDepModules = new TreeSet<>(Arrays.asList());

    @Option(name = "python-version",
            description = "The version of a Python interpreter to use."
                    + "Currently, only major version number is fully supported."
                    + "Example: \"2\", or \"3\".")
    private String mPythonVersion = "2";

    private IBuildInfo mBuildInfo = null;
    private DeviceDescriptor mDescriptor = null;
    private IRunUtil mRunUtil = new RunUtil();

    String mLocalPypiPath = null;
    String mPipPath = null;

    // Since we allow virtual env path to be reused during a test plan/module, only the preparer
    // which created the directory should be the one to delete it.
    private boolean mIsDirCreator = false;

    // If the same object is used in multiple threads (in sharding mode), the class
    // needs to know when it is safe to call the teardown method.
    private int mNumOfInstances = 0;

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void setUp(IInvocationContext context)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        ++mNumOfInstances;
        mBuildInfo = context.getBuildInfos().get(0);
        if (mNumOfInstances == 1) {
            CLog.i("Preparing python dependencies...");
            ITestDevice device = context.getDevices().get(0);
            mDescriptor = device.getDeviceDescriptor();
            createVirtualenv(mBuildInfo);
            VtsPythonRunnerHelper.activateVirtualenv(getRunUtil(), mVenvDir.getAbsolutePath());
            setLocalPypiPath();
            installDeps();
        }
        addPathToBuild(mBuildInfo);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void tearDown(IInvocationContext context, Throwable e)
            throws DeviceNotAvailableException {
        --mNumOfInstances;
        if (mNumOfInstances > 0) {
            // Since this is a host side preparer, no need to repeat
            return;
        }
        if (mVenvDir != null && mIsDirCreator) {
            try {
                recursiveDelete(mVenvDir.toPath());
                CLog.d("Deleted the virtual env's temp working dir, %s.", mVenvDir);
            } catch (IOException exception) {
                CLog.e("Failed to delete %s: %s", mVenvDir, exception);
            }
            mVenvDir = null;
        }
    }

    /**
     * This method sets mLocalPypiPath, the local PyPI package directory to
     * install python packages from in the installDeps method.
     */
    protected void setLocalPypiPath() {
        VtsVendorConfigFileUtil configReader = new VtsVendorConfigFileUtil();
        if (configReader.LoadVendorConfig(mBuildInfo)) {
            // First try to load local PyPI directory path from vendor config file
            try {
                String pypiPath = configReader.GetVendorConfigVariable(LOCAL_PYPI_PATH_KEY);
                if (pypiPath.length() > 0 && dirExistsAndHaveReadAccess(pypiPath)) {
                    mLocalPypiPath = pypiPath;
                    CLog.d(String.format("Loaded %s: %s", LOCAL_PYPI_PATH_KEY, mLocalPypiPath));
                }
            } catch (NoSuchElementException e) {
                /* continue */
            }
        }

        // If loading path from vendor config file is unsuccessful,
        // check local pypi path defined by LOCAL_PYPI_PATH_ENV_VAR_NAME
        if (mLocalPypiPath == null) {
            CLog.d("Checking whether local pypi packages directory exists");
            String pypiPath = System.getenv(LOCAL_PYPI_PATH_ENV_VAR_NAME);
            if (pypiPath == null) {
                CLog.d("Local pypi packages directory not specified by env var %s",
                        LOCAL_PYPI_PATH_ENV_VAR_NAME);
            } else if (dirExistsAndHaveReadAccess(pypiPath)) {
                mLocalPypiPath = pypiPath;
                CLog.d("Set local pypi packages directory to %s", pypiPath);
            }
        }

        if (mLocalPypiPath == null) {
            CLog.d("Failed to set local pypi packages path. Therefore internet connection to "
                    + "https://pypi.python.org/simple/ must be available to run VTS tests.");
        }
    }

    /**
     * This method returns whether the given path is a dir that exists and the user has read access.
     */
    private boolean dirExistsAndHaveReadAccess(String path) {
        File pathDir = new File(path);
        if (!pathDir.exists() || !pathDir.isDirectory()) {
            CLog.d("Directory %s does not exist.", pathDir);
            return false;
        }

        if (!EnvUtil.isOnWindows()) {
            CommandResult c = getRunUtil().runTimedCmd(BASE_TIMEOUT * 5, "ls", path);
            if (c.getStatus() != CommandStatus.SUCCESS) {
                CLog.d(String.format("Failed to read dir: %s. Result %s. stdout: %s, stderr: %s",
                        path, c.getStatus(), c.getStdout(), c.getStderr()));
                return false;
            }
            return true;
        } else {
            try {
                String[] pathDirList = pathDir.list();
                if (pathDirList == null) {
                    CLog.d("Failed to read dir: %s. Please check access permission.", pathDir);
                    return false;
                }
            } catch (SecurityException e) {
                CLog.d(String.format(
                        "Failed to read dir %s with SecurityException %s", pathDir, e));
                return false;
            }
            return true;
        }
    }

    protected void installDeps() throws TargetSetupError {
        boolean hasDependencies = false;
        if (!mScriptFiles.isEmpty()) {
            for (String scriptFile : mScriptFiles) {
                CLog.d("Attempting to execute a script, %s", scriptFile);
                CommandResult c = getRunUtil().runTimedCmd(BASE_TIMEOUT * 5, scriptFile);
                if (c.getStatus() != CommandStatus.SUCCESS) {
                    CLog.e("Executing script %s failed", scriptFile);
                    throw new TargetSetupError("Failed to source a script", mDescriptor);
                }
            }
        }
        if (mRequirementsFile != null) {
            CommandResult c = getRunUtil().runTimedCmd(BASE_TIMEOUT * 5, getPipPath(), "install",
                    "-r", mRequirementsFile.getAbsolutePath());
            if (!CommandStatus.SUCCESS.equals(c.getStatus())) {
                CLog.e("Installing dependencies from %s failed with error: %s",
                        mRequirementsFile.getAbsolutePath(), c.getStderr());
                throw new TargetSetupError("Failed to install dependencies with pip", mDescriptor);
            }
            hasDependencies = true;
        }
        if (!mDepModules.isEmpty()) {
            for (String dep : mDepModules) {
                if (mNoDepModules.contains(dep)) {
                    continue;
                }
                CommandResult result = null;
                if (mLocalPypiPath != null) {
                    CLog.d("Attempting installation of %s from local directory", dep);
                    result = getRunUtil().runTimedCmd(BASE_TIMEOUT * 5, getPipPath(), "install",
                            dep, "--no-index", "--find-links=" + mLocalPypiPath);
                    CLog.d(String.format("Result %s. stdout: %s, stderr: %s", result.getStatus(),
                            result.getStdout(), result.getStderr()));
                    if (result.getStatus() != CommandStatus.SUCCESS) {
                        CLog.e(String.format("Installing %s from %s failed", dep, mLocalPypiPath));
                    }
                }
                if (mLocalPypiPath == null || result.getStatus() != CommandStatus.SUCCESS) {
                    CLog.d("Attempting installation of %s from PyPI", dep);
                    result = getRunUtil().runTimedCmd(
                            BASE_TIMEOUT * 5, getPipPath(), "install", dep);
                    CLog.d("Result %s. stdout: %s, stderr: %s", result.getStatus(),
                            result.getStdout(), result.getStderr());
                    if (result.getStatus() != CommandStatus.SUCCESS) {
                        CLog.e("Installing %s from PyPI failed.", dep);
                        CLog.d("Attempting to upgrade %s", dep);
                        result = getRunUtil().runTimedCmd(
                                BASE_TIMEOUT * 5, getPipPath(), "install", "--upgrade", dep);
                        if (result.getStatus() != CommandStatus.SUCCESS) {
                            throw new TargetSetupError(
                                    String.format("Failed to install dependencies with pip. "
                                                    + "Result %s. stdout: %s, stderr: %s",
                                            result.getStatus(), result.getStdout(),
                                            result.getStderr()),
                                    mDescriptor);
                        } else {
                            CLog.d(String.format("Result %s. stdout: %s, stderr: %s",
                                    result.getStatus(), result.getStdout(), result.getStderr()));
                        }
                    }
                }
                hasDependencies = true;
            }
        }
        if (!hasDependencies) {
            CLog.d("No dependencies to install");
        }
    }

    /**
     * This method returns absolute pip path in virtualenv.
     *
     * This method is needed because although PATH is set in IRunUtil, IRunUtil will still
     * use pip from system path.
     *
     * @return absolute pip path in virtualenv. null if virtualenv not available.
     */
    public String getPipPath() {
        if (mPipPath != null) {
            return mPipPath;
        }

        String virtualenvPath = mVenvDir.getAbsolutePath();
        if (virtualenvPath == null) {
            return null;
        }
        mPipPath = new File(VtsPythonRunnerHelper.getPythonBinDir(virtualenvPath), "pip")
                           .getAbsolutePath();
        return mPipPath;
    }

    /**
     * Get the major python version from option.
     *
     * Currently, only 2 and 3 are supported.
     *
     * @return major version number
     * @throws TargetSetupError
     */
    protected int getConfiguredPythonVersionMajor() throws TargetSetupError {
        if (mPythonVersion.startsWith("3.") || mPythonVersion.equals("3")) {
            return 3;
        } else if (mPythonVersion.startsWith("2.") || mPythonVersion.equals("2")) {
            return 2;
        } else {
            throw new TargetSetupError("Unsupported python version " + mPythonVersion);
        }
    }

    /**
     * Add PYTHONPATH and VIRTUAL_ENV_PATH to BuildInfo.
     * @param buildInfo
     * @throws TargetSetupError
     */
    protected void addPathToBuild(IBuildInfo buildInfo) throws TargetSetupError {
        String target = null;
        switch (getConfiguredPythonVersionMajor()) {
            case 2:
                target = VtsPythonVirtualenvPreparer.VIRTUAL_ENV;
                break;
            case 3:
                target = VtsPythonVirtualenvPreparer.VIRTUAL_ENV_V3;
                break;
        }

        if (buildInfo.getFile(target) == null) {
            buildInfo.setFile(target, new File(mVenvDir.getAbsolutePath()), buildInfo.getBuildId());
        }
    }

    /**
     * Create virtualenv directory by executing virtualenv command.
     * @param buildInfo
     * @throws TargetSetupError
     */
    protected void createVirtualenv(IBuildInfo buildInfo) throws TargetSetupError {
        if (mVenvDir == null) {
            switch (getConfiguredPythonVersionMajor()) {
                case 2:
                    mVenvDir = buildInfo.getFile(VtsPythonVirtualenvPreparer.VIRTUAL_ENV);
                    break;
                case 3:
                    mVenvDir = buildInfo.getFile(VtsPythonVirtualenvPreparer.VIRTUAL_ENV_V3);
                    break;
            }
        }

        if (mVenvDir == null) {
            CLog.d("Creating virtualenv for version " + mPythonVersion);
            try {
                mVenvDir = FileUtil.createTempDir("vts-virtualenv-" + mPythonVersion + "-"
                        + VtsFileUtil.normalizeFileName(buildInfo.getTestTag()) + "_");
                mIsDirCreator = true;
                String virtualEnvPath = mVenvDir.getAbsolutePath();
                String[] cmd = new String[] {
                        "virtualenv", "-p", "python" + mPythonVersion, virtualEnvPath};
                CommandResult c = getRunUtil().runTimedCmd(BASE_TIMEOUT, cmd);
                if (c.getStatus() != CommandStatus.SUCCESS) {
                    CLog.e(String.format("Failed to create virtualenv with : %s.", virtualEnvPath));
                    CLog.e(String.format("Exit code: %s, stdout: %s, stderr: %s", c.getStatus(),
                            c.getStdout(), c.getStderr()));
                    throw new TargetSetupError("Failed to create virtualenv", mDescriptor);
                }
            } catch (IOException | RuntimeException e) {
                CLog.e("Failed to create temp directory for virtualenv");
                throw new TargetSetupError("Error creating virtualenv", e, mDescriptor);
            }
        }

        CLog.d("Python virtualenv path is: " + mVenvDir);
    }

    protected void addDepModule(String module) {
        mDepModules.add(module);
    }

    protected void setRequirementsFile(File f) {
        mRequirementsFile = f;
    }

    /**
     * Get an instance of {@link IRunUtil}.
     */
    @VisibleForTesting
    IRunUtil getRunUtil() {
        if (mRunUtil == null) {
            mRunUtil = new RunUtil();
        }
        return mRunUtil;
    }

    /**
     * This method recursively deletes a file tree without following symbolic links.
     *
     * @param rootPath the path to delete.
     * @throws IOException if fails to traverse or delete the files.
     */
    private static void recursiveDelete(Path rootPath) throws IOException {
        Files.walkFileTree(rootPath, new SimpleFileVisitor<Path>() {
            @Override
            public FileVisitResult visitFile(Path file, BasicFileAttributes attrs)
                    throws IOException {
                Files.delete(file);
                return FileVisitResult.CONTINUE;
            }
            @Override
            public FileVisitResult postVisitDirectory(Path dir, IOException e) throws IOException {
                if (e != null) {
                    throw e;
                }
                Files.delete(dir);
                return FileVisitResult.CONTINUE;
            }
        });
    }
}
