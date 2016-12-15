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

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

from vts.testcases.kernel.ltp import KernelLtpTestCase


class KernelLtpTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Runs the LTP (Linux Test Project) testcases against Android OS kernel.

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
    _TPASS = 0
    _TCONF = 32
    _32BIT = 32
    _64BIT = 64
    _KEY_ENV_TMPDIR = 'TMPDIR'
    _KEY_ENV_TMP = 'TMP'
    _KEY_ENV_LTP_DEV_FS_TYPE = 'LTP_DEV_FS_TYPE'
    _KEY_ENV_LTPROOT = 'LTPROOT'
    _KEY_ENV_PATH = 'PATH'

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)

        logging.info("data_file_path: %s", self.data_file_path)
        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self._shell = self._dut.shell.one
        self._ltp_dir = "/data/local/tmp/ltp"

        self._requirement = KernelLtpTestCase.EnvironmentRequirementChecker(self._shell)

        self._testcases = KernelLtpTestCase.TestCasesParser(self.data_file_path)
        self._env = {self._KEY_ENV_TMPDIR: KernelLtpTestCase.LTPTMP,
                     self._KEY_ENV_TMP: "%s/tmp" % KernelLtpTestCase.LTPTMP,
                     self._KEY_ENV_LTP_DEV_FS_TYPE: "ext4",
                     self._KEY_ENV_LTPROOT: self._ltp_dir,
                     self._KEY_ENV_PATH:
                     "/system/bin:%s/testcases/bin" % self._ltp_dir, }


    def PushFiles(self, n_bit):
        """Push the related files to target.

        Args:
            n_bit: int, _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """

        self._shell.Execute("mkdir %s -p" % self._ltp_dir)
        self._dut.adb.push("%s/%i/ltp/. %s" %
                           (self.data_file_path, n_bit, self._ltp_dir))
        # TODO: libcap

    def GetEnvp(self):
        """Generate the environment variable required to run the tests."""
        return ' '.join("%s=%s" % (key, value)
                        for key, value in self._env.items())

    def tearDownClass(self):
        """Deletes all copied data files."""
        self._shell.Execute("rm -rf %s" % self._ltp_dir)
        self._requirement.Cleanup()

    def Verify(self, results):
        """Verifies the test result of each test case."""

        if len(results) == 0:
            # TIMEOUT
            asserts.fail("No response received. Socket timeout")

        logging.info("stdout: %s" % str(results[const.STDOUT]))
        logging.info("stderr: %s" % str(results[const.STDERR]))
        logging.info("exit_code: %s" %
                     str(results[const.EXIT_CODE]))

        # For LTP test cases, we run one shell command for each test case
        # So the result should also contains only one execution output
        if results[const.EXIT_CODE][0] not in (self._TPASS, self._TCONF) or \
                   "TFAIL" in results[const.STDOUT][0]:
            # If return code is other than 0 or 32,
            # or the output contains 'TFAIL', then test FAIL
            asserts.fail("Test received TFAIL/TBOK or return code is neither "
                          "TPASS nor TCONF.")
        elif results[const.EXIT_CODE][0] == self._TPASS:
            # If output exit_code is _TPASS, then test PASS
            return
        elif results[const.EXIT_CODE][0] == self._TCONF and \
            "TPASS" in results[const.STDOUT][0]:
            # If output exit_code is _TCONF or but the stdout
            # contains TPASS, this means part of the test passed
            # but others were skipped. We consider it a PASS
            return
        elif results[const.EXIT_CODE][0] == self._TCONF:
            # Test case is not for the current configuration, SKIP
            asserts.skip("Incompatible test skipped: TCONF")
        else:
            # All other cases are treated as FAIL, but this is not expected
            asserts.fail("Unexpected unknown failure.")

    def TestNBits(self, n_bit):
        """Runs all 32-bit or 64-bit LTP test cases.

        Args:
            n_bit: _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """
        self.PushFiles(n_bit)

        test_cases = list(self._testcases.Load(self._ltp_dir))
        logging.info("Checking binary exists for all test cases.")
        self._requirement.RunCheckTestcasePathExistsAll(test_cases)
        self._requirement.RunChmodTestcasesAll(test_cases)
        logging.info("Start running individual tests.")

        self.runGeneratedTests(test_func=self.RunLtpOnce,
                               settings=test_cases,
                               args=(n_bit,),
                               name_func=self.GetTestName)


    def GetTestName(self, test_case, n_bit):
        "Generate the vts test name of a ltp test"
        return "{}_{}bit".format(test_case, n_bit)

    def RunLtpOnce(self, test_case, n_bit):
        "Run one LTP test case"

        if not self._requirement.Check(test_case):
            logging.info("Test case environment requirements not satisfied: %s" % test_case.note)
            asserts.skip("Test case environment requirement not satisfied. Test case skipped.")

        testcase_name = self.GetTestName(test_case, n_bit)

        logging.info("executing a test binary: [%s]" % test_case.path)
        logging.info("execution envp: [%s]" % self.GetEnvp())
        logging.info("execution params: [%s]" % \
                     test_case.GetArgs("$LTPROOT", self._ltp_dir))

        self._dut.shell.InvokeTerminal(testcase_name)
        shell = getattr(self._dut.shell, testcase_name)

        results = shell.Execute("env %s %s %s" % (
            self.GetEnvp(), test_case.path, test_case.GetArgs("$LTPROOT",
                                                    self._ltp_dir)))

        self.Verify(results)

    def test32Bits(self):
        """Runs all 32-bit LTP test cases."""
        self.TestNBits(self._32BIT)

#     def test64Bits(self):
#         # TODO: enable 64 bit tests
#         """Runs all 64-bit LTP test cases."""
#         self.TestNBits(self._64BIT)

if __name__ == "__main__":
    test_runner.main()
