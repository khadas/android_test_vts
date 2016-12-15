#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import os
import subprocess
import types

from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

from vts.testcases.kernel.ltp import KernelLtpTest
from vts.testcases.kernel.ltp import KernelLtpTestHelper


class DutWrapper(object):
    """Wrapper class for AndroidDevice object."""
    def __init__(self, dut):
        self._dut = dut
        self._shell = AdbWrapperForShell()

    @property
    def adb(self):
        """Returns original adb member"""
        return self._dut.adb

    @property
    def shell(self):
        """Returns a adb wrapped shell that mimic behavior of a shell mirror"""
        return self._shell


class AdbWrapperForShell(object):
    """A adb wrapped shell that mimic behavior of a shell mirror"""

    def InvokeTerminal(self, name):
        """Since we are using adb to execute shell commands, this method does nothing"""
        pass

    def ExecuteOneCommand(self, cmd):
        """Execute one shell command using adb and return stdout, stderr, exit_code"""
        p = subprocess.Popen("adb shell {}".format(cmd) , shell=True,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        return (stdout, stderr, p.returncode)

    def Execute(self, command):
        """Execute one shell command or a list of shell commands"""
        if not isinstance(command, types.ListType):
            command = [command]

        results = [self.ExecuteOneCommand(cmd) for cmd in command]
        stdout, stderr, exit_code = zip(*results)
        return {const.STDOUT: stdout,
                const.STDERR: stderr,
                const.EXIT_CODE: exit_code}

    def __getattr__(self, name):
        return self


class KernelLtpAdbTest(KernelLtpTest.KernelLtpTest):
    """Runs the LTP (Linux Test Project) testcases against Android OS kernel.

    This class is a child class of KernelLtpTest. The only difference is this class
    uses adb shell to execute command instead of using target shell driver.

    Attributes:
        _TPASS: int, exit_code for Test pass
        _TCONF: int, The test case is not for current configuration of kernel
        _32BIT: int, for 32 bit tests
        _64BIT: int, for 64 bit tests
        _dut: AndroidDevice, the device under test
        _shell: ShellMirrorObject, shell mirror object used to execute commands
        _ltp_dir: string, ltp build root directory on target
        _testcases: TestcasesParser, test case input parser
        _env: dict<stirng, stirng>, dict of environment variable key value pair
        _KEY_ENV__: constant strings starting with prefix "_KEY_ENV_" are used as dict
                    key in environment variable dictionary
    """
    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)

        logging.info("data_file_path: %s", self.data_file_path)
        self._dut = DutWrapper(self.registerController(android_device)[0])
        self._dut.shell.InvokeTerminal("one")
        self._shell = self._dut.shell.one
        self._ltp_dir = "/data/local/tmp/ltp"

        self._requirement = KernelLtpTestHelper.EnvironmentRequirementChecker(
            self._shell)

        self._testcases = KernelLtpTestHelper.TestCasesParser(
            self.data_file_path)
        self._env = {self._KEY_ENV_TMPDIR: KernelLtpTestHelper.LTPTMP,
                     self._KEY_ENV_TMP: "%s/tmp" % KernelLtpTestHelper.LTPTMP,
                     self._KEY_ENV_LTP_DEV_FS_TYPE: "ext4",
                     self._KEY_ENV_LTPROOT: self._ltp_dir,
                     self._KEY_ENV_PATH:
                     "/system/bin:%s/testcases/bin" % self._ltp_dir, }

if __name__ == "__main__":
    test_runner.main()
