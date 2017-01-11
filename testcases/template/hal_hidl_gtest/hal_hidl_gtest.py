#!/usr/bin/env python3.4
#
# Copyright (C) 2017 The Android Open Source Project
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

from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.testcases.template.gtest_binary_test import gtest_binary_test
from vts.utils.python.cpu import cpu_frequency_scaling


class HidlHalGTest(gtest_binary_test.GtestBinaryTest):
    '''Base class to run gtest (Google Test) binary on target.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        shell: ShellMirrorObject, shell mirror
        test_cases: list of GtestTestCase objects, list of test cases to run
        tags: all the tags that appeared in binary list
        DEVICE_TEST_DIR: string, temp location for storing binary
        TAG_PATH_SEPARATOR: string, separator used to separate tag and path
        _cpu_freq: CpuFrequencyScalingController instance of a target device.
        _skip_all_testcases: boolean - to skip all test cases. set when a target
                             device does not have a required HIDL service.
    '''

    def setUpClass(self):
        """Turns on CPU frequency scaling."""
        super(HidlHalGTest, self).setUpClass()

        opt_params = [
            keys.ConfigKeys.IKEY_HWBINDER_SERVICE
        ]
        self.getUserParams(opt_param_names=opt_params)

        hwbinder_service_name = str(getattr(
            self, keys.ConfigKeys.IKEY_HWBINDER_SERVICE, ""))
        self._skip_all_testcases = False
        if hwbinder_service_name:
            if not hwbinder_service_name.startswith("android.hardware."):
                logging.error("The given hwbinder service name %s is invalid.",
                              hwbinder_service_name)
            else:
                cmd_results = self.shell.Execute(
                    "ps -A | grep %s@" % hwbinder_service_name)
                if any(cmd_results[const.EXIT_CODE]):
                    logging.warn("The required hwbinder service %s not found.",
                                 hwbinder_service_name)
                    self._skip_all_testcases = True

        if not self._skip_all_testcases:
            self._cpu_freq = cpu_frequency_scaling.CpuFrequencyScalingController(self._dut)
            self._cpu_freq.DisableCpuScaling()

    def setUpTest(self):
        """Skips the test case if thermal throttling lasts for 30 seconds."""
        super(HidlHalGTest, self).setUpTest()
        if not self._skip_all_testcases:
            self._cpu_freq.SkipIfThermalThrottling(retry_delay_secs=30)
        else:
            logging.info("Skip a test case.")

    def tearDownTest(self):
        """Skips the test case if there is thermal throttling."""
        if not self._skip_all_testcases:
            self._cpu_freq.SkipIfThermalThrottling()
        super(HidlHalGTest, self).tearDownTest()

    def tearDownClass(self):
        """Turns off CPU frequency scaling."""
        if not self._skip_all_testcases:
            self._cpu_freq.EnableCpuScaling()
        super(HidlHalGTest, self).tearDownClass()


if __name__ == "__main__":
    test_runner.main()
