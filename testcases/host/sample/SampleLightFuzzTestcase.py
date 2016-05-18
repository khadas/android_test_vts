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
from vts.utils.python.data_objects import HalLightsDataObject
from vts.utils.python.fuzzer import GenePool


class SampleLightFuzzTestcase(base_test.BaseTestClass):
    """A sample fuzz testcase for the legacy lights HAL."""

    def testTurnOnBackgroundLight(self, module_name):
        """A fuzz testcase which calls a function using different values."""
        hal_mirror = Mirror.Mirror("/system/lib64/hw,/system/lib/hw")
        hal_mirror.InitHal("light", 1.0, module_name=module_name)
        genes = GenePool.CreateGenePool(
            10,
            HalLightsDataObject.light_state_t,
            HalLightsDataObject.light_state_t_fuzz,
            color=0xffffff00,
            flashMode=HalLightsDataObject.LIGHT_FLASH_HARDWARE,
            flashOnMs=100,
            flashOffMs=200,
            brightnessMode=HalLightsDataObject.BRIGHTNESS_MODE_USER)

        for iteration in range(10):
            index = 0
            for gene in genes:
                logging.info("Gene %d", index)
                hal_mirror.light.set_light(None, gene)
                index += 1
            genes = GenePool.Evolve(
                genes, HalLightsDataObject.light_state_t_fuzz)


def main(args):
    """Main function which calls the framework's Main()."""
    # TODO: call base_test.Main(args) instead.
    Log.SetupLogger()
    # TODO: use the test runner instead.
    testcase = SampleLightFuzzTestcase({})
    testcase.testTurnOnBackgroundLight(HalLightsDataObject.LIGHT_ID_BACKLIGHT)
    testcase.testTurnOnBackgroundLight(HalLightsDataObject.LIGHT_ID_KEYBOARD)
    testcase.testTurnOnBackgroundLight(HalLightsDataObject.LIGHT_ID_BUTTONS)
    testcase.testTurnOnBackgroundLight(HalLightsDataObject.LIGHT_ID_BATTERY)
    testcase.testTurnOnBackgroundLight(HalLightsDataObject.LIGHT_ID_NOTIFICATIONS)
    testcase.testTurnOnBackgroundLight(HalLightsDataObject.LIGHT_ID_ATTENTION)
    testcase.testTurnOnBackgroundLight(HalLightsDataObject.LIGHT_ID_BLUETOOTH)
    testcase.testTurnOnBackgroundLight(HalLightsDataObject.LIGHT_ID_WIFI)


if __name__ == "__main__":
    main(sys.argv[1:])
