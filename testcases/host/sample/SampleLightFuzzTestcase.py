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
import random
import sys
import time

from vts.runners.host import base_test
from vts.runners.host.logger import Log
from vts.utils.python.mirror_objects import Mirror
from vts.utils.python.fuzzer import GenePool


class SampleLightFuzzTestcase(base_test.BaseTestClass):
    """A sample fuzz testcase for the legacy lights HAL."""

    def testAll(self, gene_pool_size, iteartion_count):
        """tests all modules.

        Args:
          gene_pool_size: integer, the number of genes to use.
          iteartion_count: integer, the number of evolution steps.
        """
        self._gene_pool_size = gene_pool_size
        self._iteartion_count = iteartion_count

        self.hal_mirror = Mirror.Mirror("/system/lib64/hw,/system/lib/hw")
        self.hal_mirror.InitHal("light", 1.0)
        module_name = random.choice([
            self.hal_mirror.light.LIGHT_ID_BACKLIGHT,
            self.hal_mirror.light.LIGHT_ID_NOTIFICATIONS,
            self.hal_mirror.light.LIGHT_ID_ATTENTION])
        # TODO: broken on bullhead
        #   self.hal_mirror.light.LIGHT_ID_KEYBOARD
        #   self.hal_mirror.light.LIGHT_ID_BUTTONS
        #   self.hal_mirror.light.LIGHT_ID_BATTERY
        #   self.hal_mirror.light.LIGHT_ID_BLUETOOTH
        #   self.hal_mirror.light.LIGHT_ID_WIFI
        self.TestTurnOnBackgroundLight(module_name)
        logging.info("module name: %s", module_name)

    def TestTurnOnBackgroundLight(self, module_name):
        """A fuzz testcase which calls a function using different values."""
        self.hal_mirror.InitHal("light", 1.0, module_name=module_name)
        genes = GenePool.CreateGenePool(
            self._gene_pool_size,
            self.hal_mirror.light.light_state_t,
            self.hal_mirror.light.light_state_t_fuzz,
            color=0xffffff00,
            flashMode=self.hal_mirror.light.LIGHT_FLASH_HARDWARE,
            flashOnMs=100,
            flashOffMs=200,
            brightnessMode=self.hal_mirror.light.BRIGHTNESS_MODE_USER)

        for iteration in range(self._iteartion_count):
            index = 0
            for gene in genes:
                logging.info("Gene %d", index)
                self.hal_mirror.light.set_light(None, gene)
                index += 1
            genes = GenePool.Evolve(
                genes, self.hal_mirror.light.light_state_t_fuzz)


def main(args):
    """Main function which calls the framework's Main()."""
    # TODO: call base_test.Main(args) instead.
    Log.SetupLogger()
    # TODO: use the test runner instead.
    testcase = SampleLightFuzzTestcase({})
    testcase.testAll(10, 10)


if __name__ == "__main__":
    main(sys.argv[1:])
