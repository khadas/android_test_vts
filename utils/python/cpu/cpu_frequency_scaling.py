#!/usr/bin/env python
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
from vts.runners.host import const


class CpuFrequencyScalingController(object):
    """CPU Frequency Scaling Controller.

    Attributes:
        _dut: the target device DUT instance.
        _shell: Shell mirror object for communication with a target.
        _min_cpu_number: integer, the min CPU number.
        _max_cpu_number; integer, the max CPU number.
        _theoretical_max_frequency: a dict where its key is the CPU number and
                                    its value is a string containing the
                                    theoretical max CPU frequency.
    """

    def __init__(self, dut):
        self._dut = dut
        self._init = False

    def Init(self):
        """Creates a shell mirror object and reads the configuration values."""
        if self._init:
            return
        self._dut.shell.InvokeTerminal("cpu_frequency_scaling")
        self._shell = self._dut.shell.cpu_frequency_scaling
        self._min_cpu_number, self._max_cpu_number = self._GetMinAndMaxCpuNo()
        self._theoretical_max_frequency = {}
        for cpu_no in range(self._min_cpu_number, self._max_cpu_number):
            results = self._shell.Execute(
                "cat /sys/devices/system/cpu/cpu%s/"
                "cpufreq/scaling_available_frequencies" % cpu_no)
            asserts.assertEqual(1, len(results[const.STDOUT]))
            self._theoretical_max_frequency[cpu_no] = results[
                const.STDOUT][0].split()[-1].strip()
        self._init = True

    def _GetMinAndMaxCpuNo(self):
        """Returns the min and max CPU numbers.

        Returns:
            integer: min CPU number
            integer: max CPU number
        """
        results = self._shell.Execute(
            "cat /sys/devices/system/cpu/possible")
        asserts.assertEqual(len(results[const.STDOUT]), 1)
        stdout_lines = results[const.STDOUT][0].split("\n")
        (low, high) = stdout_lines[0].split('-')
        logging.info("possible cpus: %s : %s" % (low, high))
        return int(low), int(high)

    def ChangeCpuGoverner(self, mode):
        """Changes the CPU governer mode of all the CPUs on the device.

        Args:
            mode: expected CPU governer mode, e.g., 'performance' or 'interactive'.
        """
        self.Init()
        for cpu_no in range(self._min_cpu_number, self._max_cpu_number):
            self._shell.Execute(
                "echo %s > /sys/devices/system/cpu/cpu%s/"
                "cpufreq/scaling_governor" % (mode, cpu_no))

    def DisableCpuScaling(self):
        """Disable CPU frequency scaling on the device."""
        self.ChangeCpuGoverner("performance")

    def EnableCpuScaling(self):
        """Enable CPU frequency scaling on the device."""
        self.ChangeCpuGoverner("interactive")

    def IsUnderThermalThrottling(self):
        """Checks whether a target device is under thermal throttling.

        Returns:
            True if the current CPU frequency is not the theoretical max,
            False otherwise.
        """
        self.Init()
        for cpu_no in range(self._min_cpu_number, self._max_cpu_number):
            results = self._shell.Execute(
                ["cat /sys/devices/system/cpu/cpu%s/cpufreq/scaling_max_freq" % cpu_no,
                 "cat /sys/devices/system/cpu/cpu%s/cpufreq/scaling_cur_freq" % cpu_no])
            asserts.assertEqual(2, len(results[const.STDOUT]))
            asserts.assertFalse(
                any(results[const.EXIT_CODE]),
                "Can't check the current and/or max CPU frequency.")
            configurable_max_frequency = results[const.STDOUT][0].strip()
            current_frequency = results[const.STDOUT][1].strip()
            if configurable_max_frequency != current_frequency:
                logging.error(
                    "CPU%s: Configurable max frequency %s != current frequency %s",
                    cpu_no, configurable_max_frequency, current_frequency)
                return True
            if self._theoretical_max_frequency[cpu_no] != current_frequency:
                logging.error(
                    "CPU%s, Theoretical max frequency %s != scaling current frequency %s",
                    cpu_no, self._theoretical_max_frequency[cpu_no], current_frequency)
                return True
        return False

    def SkipIfThermalThrottling(self, retry_delay_secs=0):
        """Skips the current test case if a target device is under thermal throttling.

        Args:
            retry_delay_secs: integer, if not 0, retry after the specified seconds.
        """
        throttling = self.IsUnderThermalThrottling()
        if throttling and retry_delay_secs > 0:
            logging.info("Wait for %s seconds for the target to cool down.",
                         retry_delay_secs)
            time.delay(retry_delay_secs)
            throttling = self.IsUnderThermalThrottling()
        asserts.skipIf(throttling, "Thermal throttling")

