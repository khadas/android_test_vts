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
import os

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.coverage import coverage_utils


class HalHidlReplayTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Base class to run a HAL HIDL replay test on a target device.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        shell: ShellMirrorObject, shell mirror
        DEVICE_TMP_DIR: string, target device's tmp directory path.
        DEVICE_TRACE_FILE_PATH: string, the path of a trace file stored in
                                a target device.
        DEVICE_VTS_SPEC_FILE_PATH: string, the path of a VTS spec file
                                   stored in a target device.
    """

    DEVICE_TMP_DIR = "/data/local/tmp"
    DEVICE_TRACE_FILE_PATH = "/data/local/tmp/vts_replay_trace/trace_file.vts.trace"
    DEVICE_VTS_SPEC_FILE_PATH = "/data/local/tmp/spec/target.vts"

    def setUpClass(self):
        """Prepares class and initializes a target device."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_HAL_HIDL_REPLAY_TEST_TRACE_PATHS,
            keys.ConfigKeys.IKEY_HAL_HIDL_PACKAGE_NAME,
            keys.ConfigKeys.IKEY_ABI_BITNESS
        ]
        opt_params = [
            keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK,
        ]
        self.getUserParams(
            req_param_names=required_params, opt_param_names=opt_params)

        self.hal_hidl_package_name = str(self.hal_hidl_package_name)
        self.abi_bitness = str(self.abi_bitness)

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self.shell = self._dut.shell.one

        if getattr(self, keys.ConfigKeys.IKEY_ENABLE_COVERAGE, False):
            coverage_utils.InitializeDeviceCoverage(self._dut)

        self.shell.Execute("setenforce 0")  # SELinux permissive mode

        # Stop Android runtime to reduce interference.
        if getattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK,
                   False):
            self._dut.stop()

    def testReplay(self):
        """Replays the given trace(s)."""
        # TODO(yim): Use the InitHidlHal host-side stub and a stateful session
        #            to control the VTS driver (replayer mode).

        target_package, target_version = self.hal_hidl_package_name.split("@")
        for trace_path in self.hal_hidl_replay_test_trace_paths:
            trace_path = str(trace_path)
            self._dut.adb.push(
                os.path.join(self.data_file_path, "hal-hidl-trace", trace_path),
                "/data/local/tmp/vts_replay_trace/trace_file.vts.trace")

            custom_ld_library_path = os.path.join(self.DEVICE_TMP_DIR, self.abi_bitness)
            driver_binary_path = os.path.join(
                self.DEVICE_TMP_DIR, self.abi_bitness, "fuzzer%s" % self.abi_bitness)
            target_vts_driver_file_path = os.path.join(
                self.DEVICE_TMP_DIR, self.abi_bitness,
                "%s.vts.driver@%s.so" % (target_package, target_version))

            cmd = ("LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH "
                   "%s "
                   "--mode=replay "
                   "--trace_path=%s "
                   "--spec_path=%s "
                   "--target_package=%s "
                   "%s" % (custom_ld_library_path,
                           driver_binary_path,
                           self.DEVICE_TRACE_FILE_PATH,
                           self.DEVICE_VTS_SPEC_FILE_PATH,
                           target_package,
                           target_vts_driver_file_path))

            logging.info("Executing replay test command: %s", cmd)
            command_results = self.shell.Execute(cmd)

            asserts.assertTrue(command_results, "Empty command response.")
            asserts.assertFalse(
                any(command_results[const.EXIT_CODE]),
                "Test failed with the following results: %s" % command_results)

    def tearDownClass(self):
        """Performs clean-up tasks."""
        # Restart Android runtime.
        if getattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK,
                   False):
            self._dut.start()

        # Retrieve coverage if applicable
        if getattr(self, keys.ConfigKeys.IKEY_ENABLE_COVERAGE, False):
            gcda_dict = coverage_utils.GetGcdaDict(self._dut)
            self.SetCoverageData(gcda_dict, True)

        # Delete the pushed file.
        cmd_results = self.shell.Execute(
            "rm -f %s" % self.DEVICE_TRACE_FILE_PATH)
        if not cmd_results or any(cmd_results[const.EXIT_CODE]):
            logging.warning("Failed to remove: %s", cmd_results)


if __name__ == "__main__":
    test_runner.main()
