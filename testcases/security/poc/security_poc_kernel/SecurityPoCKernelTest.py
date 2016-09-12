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
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class SecurityPoCKernelTest(base_test.BaseTestClass):
    """All security PoC kernel test cases."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self.shell = self.dut.shell.one

    def test_28838221_kernel_sound_64bit(self):
        """A test case for b/28838221 which is from kernel sound driver."""
        binary = "/data/local/tmp/64/28838221_poc64"
        logging.info("device type: %s", self.dut.model)
        # TODO: customize arg based on self.dut.model.
        if self.dut.model == "unknown":
            asserts.skip("skip test_28838221_kernel_sound_64bit for a unknown device.")
            return
        arg = "/sys/kernel/debug/asoc/msm8994-tomtom-snd-card/snd-soc-dummy/codec_reg"
        results = self.shell.Execute(
            ["chmod 755 %s" % binary,
             "%s %s" % (binary, arg)])
        logging.info(str(results[const.STDOUT]))
        # TODO: enable with a correct verification logic
        # asserts.assertEqual(
        #     results[const.STDOUT][0].strip(), "expected stdout")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)  # checks the exit code

    def test_30149612_kernel_bluetooth_64bit(self):
        """A test case for b/28838221 which is from kernel bluetooth driver."""
        binary = "/data/local/tmp/64/30149612_poc64"
        results = self.shell.Execute(
            ["chmod 755 %s" % binary,
             binary])
        logging.info(str(results[const.STDOUT]))
        # TODO: enable with a correct verification logic
        # asserts.assertEqual(
        #     results[const.STDOUT][0].strip(), "expected stdout")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)  # checks the exit code


if __name__ == "__main__":
    test_runner.main()
