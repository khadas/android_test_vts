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
import time

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class VibratorHidlTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """A simple testcase for the VIBRATOR HIDL HAL."""

    _TREBLE_DEVICE_NAME_SUFFIX = "_treble"

    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer VIBRATOR service."""
        self.dut = self.registerController(android_device)[0]
        self.dut.hal.InitHidlHal(
            target_type="vibrator",
            target_basepaths=["/system/lib64"],
            target_version=1.0,
            target_package="android.hardware.vibrator",
            target_component_name="IVibrator",
            bits=64)

        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode

    def testVibratorBasic(self):
        """A simple test case which just calls each registered function."""
        asserts.skipIf(
                not self.dut.model.endswith(self._TREBLE_DEVICE_NAME_SUFFIX),
                "a non-Treble device.")

        result = self.dut.hal.vibrator.on()
        logging.info("open result: %s", result)

        time.sleep(1)

        result = self.dut.hal.vibrator.off()
        logging.info("prediscover result: %s", result)


if __name__ == "__main__":
    test_runner.main()
