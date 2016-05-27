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

from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.mirror_objects import mirror


class SampleLightTest(base_test.BaseTestClass):
    """A sample testcase for the legacy lights HAL."""

    def setUpClass(self):
        self.hal_mirror = mirror.Mirror(["/system/lib64/hw", "/system/lib/hw"])
        self.hal_mirror.InitHal("light", 1.0)
        self.hal_mirror.light.Open("backlight")

    def testTurnOnBackgroundLight(self):
        """A simple testcase which just calls a function."""
        # TODO: support ability to test non-instrumented hals.
        arg = self.hal_mirror.light.light_state_t(
            color=0xffffff00,
            flashMode=self.hal_mirror.light.LIGHT_FLASH_HARDWARE,
            flashOnMs=100,
            flashOffMs=200,
            brightnessMode=self.hal_mirror.light.BRIGHTNESS_MODE_USER)
        self.hal_mirror.light.set_light(None, arg)

    def testTurnOnBackgroundLightUsingInstrumentedLib(self):
        """A simple testcase which just calls a function."""
        arg = self.hal_mirror.light.light_state_t(
            color=0xffffff00,
            flashMode=self.hal_mirror.light.LIGHT_FLASH_HARDWARE,
            flashOnMs=100,
            flashOffMs=200,
            brightnessMode=self.hal_mirror.light.BRIGHTNESS_MODE_USER)
        logging.info(self.hal_mirror.light.set_light(None, arg))


if __name__ == "__main__":
    test_runner.main()
