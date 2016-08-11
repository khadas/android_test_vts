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

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

from vts.testcases.kernel.ltp import test_cases_parser
from vts.testcases.kernel.ltp import environment_requirement_checker as env_checker
from vts.testcases.kernel.ltp import ltp_enums
from vts.testcases.kernel.ltp import ltp_configs


class KernelLtpTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Runs the LTP (Linux Test Project) testcases against Android OS kernel.

    Attributes:
        _dut: AndroidDevice, the device under test
        _shell: ShellMirrorObject, shell mirror object used to execute commands
        _testcases: TestcasesParser, test case input parser
        _env: dict<stirng, stirng>, dict of environment variable key value pair
    """
    _32BIT = "32"
    _64BIT = "64"

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                           ltp_enums.ConfigKeys.RUN_STAGING]
        self.getUserParams(required_params)

        logging.info("data_file_path: %s", self.data_file_path)
        logging.info("run_staging: %s", self.run_staging)

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self._shell = self._dut.shell.one

        self._requirement = env_checker.EnvironmentRequirementChecker(
            self._shell)

        self._testcases = test_cases_parser.TestCasesParser(
            self.data_file_path)
        self._env = {ltp_enums.ShellEnvKeys.TMP: ltp_configs.TMP,
                     ltp_enums.ShellEnvKeys.TMPBASE: ltp_configs.TMPBASE,
                     ltp_enums.ShellEnvKeys.LTPTMP: ltp_configs.LTPTMP,
                     ltp_enums.ShellEnvKeys.TMPDIR: ltp_configs.TMPDIR,
                     ltp_enums.ShellEnvKeys.LTP_DEV_FS_TYPE:
                     ltp_configs.LTP_DEV_FS_TYPE,
                     ltp_enums.ShellEnvKeys.LTPROOT: ltp_configs.LTPDIR,
                     ltp_enums.ShellEnvKeys.PATH: ltp_configs.PATH}

    def PushFiles(self, n_bit):
        """Push the related files to target.

        Args:
            n_bit: int, _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """

        self._shell.Execute("mkdir %s -p" % ltp_configs.LTPDIR)
        self._dut.adb.push("%s/%s/ltp/. %s" %
                           (self.data_file_path, n_bit, ltp_configs.LTPDIR))

    def GetEnvp(self):
        """Generate the environment variable required to run the tests."""
        return ' '.join("%s=%s" % (key, value)
                        for key, value in self._env.items())

    def tearDownClass(self):
        """Deletes all copied data files."""
        self._shell.Execute("rm -rf %s" % ltp_configs.LTPDIR)
        self._requirement.Cleanup()

    def Verify(self, results):
        """Verifies the test result of each test case."""
        asserts.assertFalse(
            len(results) == 0, "No response received. Socket timeout")

        logging.info("stdout: %s", results[const.STDOUT])
        logging.info("stderr: %s", results[const.STDERR])
        logging.info("exit_code: %s", results[const.EXIT_CODE])

        # For LTP test cases, we run one shell command for each test case
        # So the result should also contains only one execution output
        stdout = results[const.STDOUT][0]
        ret_code = results[const.EXIT_CODE][0]
        # Test case is not for the current configuration, SKIP
        if ret_code == ltp_enums.TestExitCode.TCONF:
            asserts.skipIf('TPASS' not in stdout,
                           "Incompatible test skipped: TCONF")
        else:
            asserts.assertEqual(ret_code, ltp_enums.TestExitCode.TPASS,
                                "Got return code %s, test did not pass." %
                                ret_code)

    def TestNBits(self, n_bit):
        """Runs all 32-bit or 64-bit LTP test cases.

        Args:
            n_bit: _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """
        self.PushFiles(n_bit)
        logging.info("[Test Case] test%sBits SKIP", n_bit)

        test_cases = list(
            self._testcases.Load(
                ltp_configs.LTPDIR, n_bit=n_bit, run_staging=self.run_staging))
        logging.info("Checking binary exists for all test cases.")
        self._requirement.RunCheckTestcasePathExistsAll(test_cases)
        self._requirement.RunChmodTestcasesAll(test_cases)
        logging.info("Start running %i individual tests." % len(test_cases))

        self.runGeneratedTests(
            test_func=self.RunLtpOnce,
            settings=test_cases,
            args=(n_bit, ),
            name_func=self.GetTestName)
        logging.info("[Test Case] test%sBits", n_bit)
        asserts.skip("Finished generating {} bit tests.".format(n_bit))

    def GetTestName(self, test_case, n_bit):
        "Generate the vts test name of a ltp test"
        return "{}_{}bit".format(test_case, n_bit)

    def RunLtpOnce(self, test_case, n_bit):
        "Run one LTP test case"
        self._requirement.Check(test_case)

        cmd = "env {envp} {binary} {args}".format(
            envp=self.GetEnvp(),
            binary=test_case.path,
            args=test_case.GetArgs("$LTPROOT", ltp_configs.LTPDIR))
        logging.info("Executing %s", cmd)
        self.Verify(self._shell.Execute(cmd))

    def test32Bits(self):
        """Runs all 32-bit LTP test cases."""
        self.TestNBits(self._32BIT)

#     def test64Bits(self):
#         # TODO: enable 64 bit tests
#         """Runs all 64-bit LTP test cases."""
#         self.TestNBits(self._64BIT)

if __name__ == "__main__":
    test_runner.main()
