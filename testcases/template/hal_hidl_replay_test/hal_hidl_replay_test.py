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
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.common import vintf_utils
from vts.utils.python.controllers import android_device
from vts.utils.python.os import path_utils


class HalHidlReplayTest(base_test.BaseTestClass):
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
    DEVICE_VTS_SPEC_FILE_PATH = "/data/local/tmp/spec"

    def setUpClass(self):
        """Prepares class and initializes a target device."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_HAL_HIDL_REPLAY_TEST_TRACE_PATHS,
            keys.ConfigKeys.IKEY_HAL_HIDL_PACKAGE_NAME,
            keys.ConfigKeys.IKEY_ABI_BITNESS
        ]
        opt_params = [keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK]
        self.getUserParams(
            req_param_names=required_params, opt_param_names=opt_params)

        self.hal_hidl_package_name = str(self.hal_hidl_package_name)
        self.abi_bitness = str(self.abi_bitness)

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self.shell = self._dut.shell.one

        if self.coverage.enabled:
            self.coverage.LoadArtifacts()
            self.coverage.InitializeDeviceCoverage(self._dut)

        self.shell.Execute("setenforce 0")  # SELinux permissive mode

        # Stop Android runtime to reduce interference.
        if getattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK,
                   False):
            self._dut.stop()

    def getServiceName(self):
        """Get service name(s) for the given hal."""
        service_names = set()
        vintf_xml = self._dut.getVintfXml()
        if not vintf_xml:
            logging.error("fail to get vintf xml file")
            return service_names
        hwbinder_hals, passthrough_hals = vintf_utils.GetHalDescriptions(
            vintf_xml)
        if not hwbinder_hals and not passthrough_hals:
            logging.error("fail to get hal descriptions")
            return service_names
        hwbinder_hal_info = hwbinder_hals.get(self.hal_hidl_package_name)
        passthrough_hal_info = passthrough_hals.get(self.hal_hidl_package_name)
        if not hwbinder_hal_info and not passthrough_hal_info:
            logging.error("hal %s does not exit", self.hal_hidl_package_name)
            return service_names
        if hwbinder_hal_info:
            for hal_interface in hwbinder_hal_info.hal_interfaces:
                for hal_interface_instance in hal_interface.hal_interface_instances:
                    service_names.add(hal_interface_instance)
        if passthrough_hal_info:
            for hal_interface in passthrough_hal_info.hal_interfaces:
                for hal_interface_instance in hal_interface.hal_interface_instances:
                    service_names.add(hal_interface_instance)
        return service_names

    def testReplay(self):
        """Replays the given trace(s)."""
        # TODO(yim): Use the InitHidlHal host-side stub and a stateful session
        #            to control the VTS driver (replayer mode).
        target_package, target_version = self.hal_hidl_package_name.split("@")
        for trace_path in self.hal_hidl_replay_test_trace_paths:
            trace_path = str(trace_path)
            self._dut.adb.push(
                os.path.join(self.data_file_path, "hal-hidl-trace",
                             trace_path),
                "/data/local/tmp/vts_replay_trace/trace_file.vts.trace")

            custom_ld_library_path = path_utils.JoinTargetPath(
                self.DEVICE_TMP_DIR, self.abi_bitness)
            driver_binary_path = path_utils.JoinTargetPath(
                self.DEVICE_TMP_DIR, self.abi_bitness,
                "fuzzer%s" % self.abi_bitness)
            target_vts_driver_file_path = path_utils.JoinTargetPath(
                self.DEVICE_TMP_DIR, self.abi_bitness,
                "%s@%s-vts.driver.so" % (target_package, target_version))
            service_names = self.getServiceName()
            for service_name in service_names:
                cmd = ("LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH "
                       "%s "
                       "--mode=replay "
                       "--trace_path=%s "
                       "--spec_path=%s "
                       "--hal_service_name=%s "
                       "%s" % (custom_ld_library_path, driver_binary_path,
                               self.DEVICE_TRACE_FILE_PATH,
                               self.DEVICE_VTS_SPEC_FILE_PATH, service_name,
                               target_vts_driver_file_path))

                logging.info("Executing replay test command: %s", cmd)
                command_results = self.shell.Execute(cmd)

                asserts.assertTrue(command_results, "Empty command response.")
                asserts.assertFalse(
                    any(command_results[const.EXIT_CODE]),
                    "Test failed with the following results: %s" %
                    command_results)

    def tearDownClass(self):
        """Performs clean-up tasks."""
        # Restart Android runtime.
        if getattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK,
                   False):
            self._dut.start()

        # Retrieve coverage if applicable
        if self.coverage.enabled:
            self.coverage.SetCoverageData(dut=self._dut, isGlobal=True)

        # Delete the pushed file.
        cmd_results = self.shell.Execute("rm -f %s" %
                                         self.DEVICE_TRACE_FILE_PATH)
        if not cmd_results or any(cmd_results[const.EXIT_CODE]):
            logging.warning("Failed to remove: %s", cmd_results)


if __name__ == "__main__":
    test_runner.main()
