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

import json
import logging
import os

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

from vts.testcases.security.poc.host import poc_test_config as config

class SecurityPoCKernelTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Runs security PoC kernel test cases.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        _shell: ShellMirrorObject, shell mirror
        _testcases: string list, list of testcases to run
        _model: string, device model e.g. "Nexus 5X"
        _host_input: dict, info passed to PoC test
        _test_flags: string, flags that will be passed to PoC test
    """
    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            config.ConfigKeys.RUN_STAGING
        ]
        self.getUserParams(required_params)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            self.data_file_path)

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self._shell = self._dut.shell.one
        self._testcases = config.POC_TEST_CASES_STABLE
        if self.run_staging:
            self._testcases += config.POC_TEST_CASES_STAGING

        self._host_input = self.CreateHostInput()

        self._test_flags = ["--%s \"%s\"" % (k, v) for k, v in self._host_input.items()]
        self._test_flags = " ".join(self._test_flags)
        logging.info("Test flags: %s", self._test_flags)

    def tearDownClass(self):
        """Deletes all copied data."""
        self._shell.Execute("rm -rf %s" % config.POC_TEST_DIR)

    def CreateHostInput(self):
        """Gathers information that will be passed to target-side code.

        Returns:
            host_input: dict, information passed to native PoC test.
        """
        out = self._shell.Execute("getprop | grep ro.product.model")
        device_model = out[const.STDOUT][0].strip().split('[')[-1][:-1]

        host_input = {
            "device_model": device_model,
        }
        return host_input

    def PushFiles(self):
        """adb pushes related file to target."""
        self._shell.Execute("mkdir %s -p" % config.POC_TEST_DIR)

        push_src = os.path.join(self.data_file_path, "security", "poc", ".")
        self._dut.adb.push("%s %s" % (push_src, config.POC_TEST_DIR))

    def IsRelevant(self, testcase):
        """Returns True iff testcase should run according to its config.

        Args:
            testcase: string, format testsuite/testname, specifies which
                test case to examine.
        """
        test_config_path = os.path.join(
            self.data_file_path, "security", "poc", testcase + ".config")

        with open(test_config_path) as test_config_file:
            test_config = json.load(test_config_file)
            target_models = test_config["target_models"]
            return self._host_input["device_model"] in target_models

    def RunTestcase(self, testcase):
        """Runs the given testcase and asserts the result.

        Args:
            testcase: string, format testsuite/testname, specifies which
                test case to run.
        """
        asserts.skipIf(not self.IsRelevant(testcase),
            "%s not configured to run against this target model." % testcase)

        items = testcase.split("/", 1)
        testsuite = items[0]

        chmod_cmd = "chmod -R 755 %s" % os.path.join(config.POC_TEST_DIR, testsuite)
        cd_cmd = "cd %s" % os.path.join(config.POC_TEST_DIR, testsuite)
        test_cmd = "./%s" % items[1]

        cmd = [
            chmod_cmd,
            "%s && %s %s" % (cd_cmd, test_cmd, self._test_flags)
        ]
        logging.info("Executing: %s", cmd)

        result = self._shell.Execute(cmd)
        self.AssertTestResult(result)

    def AssertTestResult(self, result):
        """Asserts that testcase finished as expected.

        Checks that device is in responsive state. If not, waits for boot
        then reports test as failure. If it is, asserts that all test commands
        returned exit code 0.

        Args:
            result: dict([str],[str],[int]), command results from shell.
        """
        if self._dut.hasBooted():
            exit_codes = result[const.EXIT_CODE]
            logging.info("EXIT_CODE: %s", exit_codes)

            # Last exit code is the exit code of PoC executable.
            asserts.skipIf(exit_codes[-1] == ExitCode.POC_TEST_SKIP,
                "%s test case was skipped." % testcase)
            asserts.assertFalse(
                any(exit_codes), "Test case failed.")
        else:
            self._dut.waitForBootCompletion()
            asserts.fail("Test case left the device in unresponsive state.")

    def generateSecurityPoCTests(self):
        """Runs security PoC tests."""
        self.PushFiles()
        self.runGeneratedTests(
            test_func=self.RunTestcase,
            settings=self._testcases,
            name_func=lambda x: x.replace('/','_'))

if __name__ == "__main__":
    test_runner.main()
