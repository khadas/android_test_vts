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
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

from vts.testcases.kernel.cpu_profiling import cpu_profiling_test_config as config

class CpuProfilingTest(base_test.BaseTestClass):
    """Runs cpu profiling test cases against Android OS kernel.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        _shell: ShellMirrorObject, shell mirror
        _testcases: string list, list of testcases to run
    """
    _32BIT = "32"
    _64BIT = "64"

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            "AndroidDevice",
        ]
        self.getUserParams(required_params)

        self.product_type = self.AndroidDevice[0]['product_type']
        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            self.data_file_path)

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self._shell = self._dut.shell.one
        self._testcases = [config.CPT_HOTPLUG_TESTSUITE + ':' + test \
                           for test in config.CPT_HOTPLUG_BASIC_TESTS]

    def RunTestcase(self, testcase, test_dir):
        """Runs the given testcase and asserts the result.

        Args:
            testcase: string, format testsuite/testname, specifies which
                test case to run.
        """
        if self.product_type in config.CPT_HOTPLUG_EXCLUDE_DEVICES:
          asserts.skip("Skip test on device {}.".format(self.product_type))

        items = testcase.split(":", 1)
        testsuite = items[0]

        chmod_cmd = "chmod -R 755 %s" % os.path.join(test_dir, testsuite)
        cd_cmd = "cd %s" % test_dir
        test_cmd = "./%s --gtest_filter=%s" % (testsuite, items[1])

        cmd = [
            chmod_cmd,
            "%s && %s" % (cd_cmd, test_cmd)
        ]
        logging.info("Executing: %s", cmd)

        result = self._shell.Execute(cmd)
        logging.info("EXIT_CODE: %s:", result[const.EXIT_CODE])

        asserts.assertFalse(
            any(result[const.EXIT_CODE]),
            "%s failed." % testcase)

    def TestNBits(self, n_bit):
        """Runs all 32-bit or all 64-bit tests.

        Args:
            n_bit: _32BIT or 32 for 32-bit tests;
                _64BIT or 64 for 64-bit tests;
        """
        logging.info("[Test Case] test%sBits SKIP", n_bit)

        self.runGeneratedTests(
            test_func=self.RunTestcase,
            settings=self._testcases,
            args=("/data/local/tmp/%s/" % n_bit, ),
            tag="%s_bit" % n_bit)
        logging.info("[Test Case] test%sBits", n_bit)
        asserts.skip("Finished generating {} bit tests.".format(n_bit))

    def test32Bits(self):
        """Runs all 32-bit tests."""
        self.TestNBits(self._32BIT)

    def test64Bits(self):
        """Runs all 64-bit tests."""
        self.TestNBits(self._64BIT)

if __name__ == "__main__":
    test_runner.main()
