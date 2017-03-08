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

    The implementation is based on the special files in
    /sys/devices/system/cpu/. CPU availability is shown in multiple files,
    including online, present, and possible. This class assumes that a CPU may
    be present but offline. If a CPU is online, its frequency scaling can be
    adjusted by reading/writing to the files in cpuX/cpufreq/ where X is the
    CPU number.

    Attributes:
        _dut: the target device DUT instance.
        _shell: Shell mirror object for communication with a target.
        _min_cpu_number: integer, the min CPU number.
        _max_cpu_number; integer, the max CPU number.
        _theoretical_max_frequency: a dict where its key is the CPU number and
                                    its value is an integer containing the
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
            if results[const.STDOUT][0]:
                freq = [int(x) for x in results[const.STDOUT][0].split()]
                self._theoretical_max_frequency[cpu_no] = max(freq)
            else:
                self._theoretical_max_frequency[cpu_no] = None
                logging.warn("cpufreq/scaling_available_frequencies for cpu %s"
                             " not set.", cpu_no)
        self._init = True

    def _GetMinAndMaxCpuNo(self):
        """Returns the min and max CPU numbers.

        Returns:
            integer: min CPU number (inclusive)
            integer: max CPU number (exclusive)
        """
        results = self._shell.Execute(
            "cat /sys/devices/system/cpu/online")
        asserts.assertEqual(len(results[const.STDOUT]), 1)
        stdout_lines = results[const.STDOUT][0].split("\n")
        stdout_split = stdout_lines[0].split('-')
        asserts.assertLess(len(stdout_split), 3)
        low = stdout_split[0]
        high = stdout_split[1] if len(stdout_split) == 2 else low
        logging.info("online cpus: %s : %s" % (low, high))
        return int(low), int(high) + 1

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
            if any(results[const.EXIT_CODE]):
                logging.error("Can't check the current and/or max CPU frequency.")
                logging.error("Stderr for scaling_max_freq: %s", results[const.STDERR][0])
                logging.error("Stderr for scaling_cur_freq: %s", results[const.STDERR][1])
                return True
            configurable_max_frequency = results[const.STDOUT][0].strip()
            current_frequency = results[const.STDOUT][1].strip()
            if configurable_max_frequency != current_frequency:
                logging.error(
                    "CPU%s: Configurable max frequency %s != current frequency %s",
                    cpu_no, configurable_max_frequency, current_frequency)
                return True
            if (self._theoretical_max_frequency[cpu_no] is not None and
                self._theoretical_max_frequency[cpu_no] != int(current_frequency)):
                logging.error(
                    "CPU%s, Theoretical max frequency %d != scaling current frequency %s",
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
            time.sleep(retry_delay_secs)
            throttling = self.IsUnderThermalThrottling()
        asserts.skipIf(throttling, "Thermal throttling")

