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
import sys
import time

from vts.runners.host import base_test
from vts.runners.host.logger import Log
from vts.utils.python.mirror_objects import Mirror


class SampleLightTestcase(base_test.BaseTestClass):
    """A sample testcase for the legacy lights HAL."""

    def testTurnOnBackgroundLight(self):
        """A simple testcase which just calls a function."""
        logging.info("testLight start")
        hal_mirror = Mirror.Mirror("/system/lib64/hw,/system/lib/hw")
        hal_mirror.InitHal("light", 1.0, module_name="backlight")
        arg = hal_mirror.light.light_state_t(
            color=0xffffff00,
            flashMode=hal_mirror.light.LIGHT_FLASH_HARDWARE,
            flashOnMs=100,
            flashOffMs=200,
            brightnessMode=hal_mirror.light.BRIGHTNESS_MODE_USER)
        hal_mirror.light.set_light(None, arg)
        time.sleep(5)
        logging.info("testLight end")


def main(args):
    """Main function which calls the framework's Main()."""
    # TODO: call base_test.Main(args) instead.
    Log.SetupLogger()
    # TODO: use the test runner instead.
    testcase = SampleLightTestcase({})
    testcase.testTurnOnBackgroundLight()


if __name__ == "__main__":
    main(sys.argv[1:])
