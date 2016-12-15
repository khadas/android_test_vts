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
import time

from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.fuzzer import GenePool
from vts.utils.python.mirror_objects import mirror


class SampleLightFuzzTest(base_test.BaseTestClass):
    """A sample fuzz testcase for the legacy lights HAL."""

    def setUpClass(self):
        required_params = ["gene_pool_size", "iteartion_count"]
        self.getUserParams(required_params)
        self.hal_mirror = mirror.Mirror(["/system/lib64/hw", "/system/lib/hw"])
        self.hal_mirror.InitHal("light", 1.0)
        module_name = random.choice(
            [self.hal_mirror.light.LIGHT_ID_BACKLIGHT,
             self.hal_mirror.light.LIGHT_ID_NOTIFICATIONS,
             self.hal_mirror.light.LIGHT_ID_ATTENTION])
        # TODO: broken on bullhead
        #   self.hal_mirror.light.LIGHT_ID_KEYBOARD
        #   self.hal_mirror.light.LIGHT_ID_BUTTONS
        #   self.hal_mirror.light.LIGHT_ID_BATTERY
        #   self.hal_mirror.light.LIGHT_ID_BLUETOOTH
        #   self.hal_mirror.light.LIGHT_ID_WIFI
        self.hal_mirror.light.Open(module_name)

    def testTurnOnLightBlackBoxFuzzing(self):
        """A fuzz testcase which calls a function using different values."""
        logging.info("blackbox fuzzing")
        genes = GenePool.CreateGenePool(
            self.gene_pool_size,
            self.hal_mirror.light.light_state_t,
            self.hal_mirror.light.light_state_t_fuzz,
            color=0xffffff00,
            flashMode=self.hal_mirror.light.LIGHT_FLASH_HARDWARE,
            flashOnMs=100,
            flashOffMs=200,
            brightnessMode=self.hal_mirror.light.BRIGHTNESS_MODE_USER)
        for iteration in range(self.iteartion_count):
            index = 0
            logging.info("blackbox iteration %d", iteration)
            for gene in genes:
                logging.debug("Gene %d", index)
                self.hal_mirror.light.set_light(None, gene)
                index += 1
            evolution = GenePool.Evolution()
            genes = evolution.Evolve(genes,
                                     self.hal_mirror.light.light_state_t_fuzz)

    def testTurnOnLightWhiteBoxFuzzing(self):
        """A fuzz testcase which calls a function using different values."""
        logging.info("whitebox fuzzing")
        genes = GenePool.CreateGenePool(
            self.gene_pool_size,
            self.hal_mirror.light.light_state_t,
            self.hal_mirror.light.light_state_t_fuzz,
            color=0xffffff00,
            flashMode=self.hal_mirror.light.LIGHT_FLASH_HARDWARE,
            flashOnMs=100,
            flashOffMs=200,
            brightnessMode=self.hal_mirror.light.BRIGHTNESS_MODE_USER)

        for iteration in range(self.iteartion_count):
            index = 0
            logging.info("whitebox iteration %d", iteration)
            coverages = []
            for gene in genes:
                logging.debug("Gene %d", index)
                result = self.hal_mirror.light.set_light(None, gene)
                if len(result.coverage_data) > 0:
                    gene_coverage = []
                    for coverage_data in result.coverage_data:
                        gene_coverage.append(coverage_data)
                    coverages.append(gene_coverage)
                index += 1
            evolution = GenePool.Evolution()
            genes = evolution.Evolve(genes,
                                     self.hal_mirror.light.light_state_t_fuzz,
                                     coverages=coverages)


if __name__ == "__main__":
    test_runner.main()
