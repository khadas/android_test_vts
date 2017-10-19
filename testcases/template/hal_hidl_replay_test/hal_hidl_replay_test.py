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
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.testcases.template.binary_test import binary_test
from vts.utils.python.hal import hal_service_name_utils
from vts.utils.python.os import path_utils


class HalHidlReplayTest(binary_test.BinaryTest):
    """Base class to run a HAL HIDL replay test on a target device.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        DEVICE_TMP_DIR: string, target device's tmp directory path.
        DEVICE_VTS_SPEC_FILE_PATH: string, target device's directory for storing
                                   HAL spec files.
    """

    DEVICE_TMP_DIR = "/data/local/tmp"
    DEVICE_VTS_SPEC_FILE_PATH = "/data/local/tmp/spec"
    DEVICE_VTS_TRACE_FILE_PATH = "/data/local/tmp/vts_replay_trace"

    def setUpClass(self):
        """Prepares class and initializes a target device."""
        super(HalHidlReplayTest, self).setUpClass()

        if self._skip_all_testcases:
            return

        if self.coverage.enabled:
            # enable passthrough mode to measure code coverage.
            self.shell.Execute('setprop vts.hidl.get_stub true')

    # @Override
    def CreateTestCases(self):
        """Create a list of HalHidlReplayTestCase objects."""
        required_params = [
            keys.ConfigKeys.IKEY_HAL_HIDL_REPLAY_TEST_TRACE_PATHS,
            keys.ConfigKeys.IKEY_ABI_BITNESS
        ]
        opt_params = [keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK]
        self.getUserParams(
            req_param_names=required_params, opt_param_names=opt_params)
        self.abi_bitness = str(self.abi_bitness)
        self.trace_paths = map(str, self.hal_hidl_replay_test_trace_paths)

        self.driver_binary_path = path_utils.JoinTargetPath(
            self.DEVICE_TMP_DIR, self.abi_bitness,
            "vts_hal_replayer%s" % self.abi_bitness)
        self.custom_ld_library_path = path_utils.JoinTargetPath(
            self.DEVICE_TMP_DIR, self.abi_bitness)

        for trace_path in self.trace_paths:
            trace_file_name = str(os.path.basename(trace_path))
            target_trace_path = path_utils.JoinTargetPath(
                self.DEVICE_VTS_TRACE_FILE_PATH, trace_file_name)
            self._dut.adb.push(
                path_utils.JoinTargetPath(self.data_file_path,
                                          "hal-hidl-trace", trace_path),
                target_trace_path)

            service_instance_combinations = self._GetServiceInstanceCombinations(
                target_trace_path)

            for instance_combination in service_instance_combinations:
                test_case = super(HalHidlReplayTest, self).CreateTestCase(
                    self.driver_binary_path, '')
                test_case.envp += "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH" % self.custom_ld_library_path
                test_case.args += " --spec_dir_path=" + self.DEVICE_VTS_SPEC_FILE_PATH
                test_case.test_name = "replay_test_" + trace_file_name
                for instance in instance_combination:
                    test_case.args += " --hal_service_instance=" + instance
                    test_case.args += " " + target_trace_path
                    test_case.tag += instance[instance.find('/'):]
                self.testcases.append(test_case)

    def VerifyTestResult(self, test_case, command_results):
        '''Parse Gtest xml result output.

        Args:
            test_case: GtestTestCase object, the test being run. This param
                       is not currently used in this method.
            command_results: dict of lists, shell command result
        '''
        asserts.assertTrue(command_results, 'Empty command response.')
        asserts.assertEqual(
            len(command_results), 3, 'Abnormal command response.')

        for stdout in command_results[const.STDOUT]:
            if stdout and stdout.strip():
                for line in stdout.split('\n'):
                    logging.info(line)

        if any(command_results[const.EXIT_CODE]):
            # print stderr only when test fails.
            for stderr in command_results[const.STDERR]:
                if stderr and stderr.strip():
                    for line in stderr.split('\n'):
                        logging.error(line)
            asserts.assertTrue(False, 'Test {} failed with the following results: {}'.format(
                test_case, command_results))


    def tearDownClass(self):
        """Performs clean-up tasks."""
        # Delete the pushed file.
        if not self._skip_all_testcases:
            for trace_path in self.trace_paths:
                trace_file_name = str(os.path.basename(trace_path))
                target_trace_path = path_utils.JoinTargetPath(
                    self.DEVICE_TMP_DIR, "vts_replay_trace", trace_file_name)
                cmd_results = self.shell.Execute("rm -f %s" %
                                                 target_trace_path)
                if not cmd_results or any(cmd_results[const.EXIT_CODE]):
                    logging.warning("Failed to remove: %s", cmd_results)

        if self.coverage.enabled:
            # disable passthrough mode.
            self.shell.Execute('setprop vts.hidl.get_stub false')
        super(HalHidlReplayTest, self).tearDownClass()

    def setUp(self):
        """Setup for code coverage for each test case."""
        super(HalHidlReplayTest, self).setUp()
        if self.coverage.enabled and not self.coverage.global_coverage:
            # enable passthrough mode to measure code coverage.
            self.shell.Execute('setprop vts.hidl.get_stub true')
            self.coverage.InitializeDeviceCoverage(self._dut)

    def tearDown(self):
        """Generate the coverage data for each test case."""
        if self.coverage.enabled and not self.coverage.global_coverage:
            self.coverage.SetCoverageData(dut=self._dut, isGlobal=False)
            # disable passthrough mode.
            self.shell.Execute('setprop vts.hidl.get_stub false')
        super(HalHidlReplayTest, self).tearDown()

    def _GetServiceInstanceCombinations(self, trace_path):
        """Create all combinations of instances for all services recorded in
        the trace file.

        Args:
            trace_path: string, full path of a given trace file
        Returns:
            A list of all service instance combinations.
        """
        registered_services = []
        service_instances = {}
        results = self.shell.Execute(
            "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --list_service %s" %
            (self.custom_ld_library_path, self.driver_binary_path, trace_path))

        if (results[const.EXIT_CODE][0]):
            logging.error('Failed to list test cases.')
            return None

        # parse the results to get the registered service list.
        for line in results[const.STDOUT][0].split('\n'):
            line = str(line)
            if line.startswith('hal_service: '):
                service = line[len('hal_service: '):]
                registered_services.append(service)

        framework_comp_matrix_xml = self._dut.getCompMatrixXml()
        framework_vintf_xml = self._dut.getVintfXml(
            use_lshal=False, is_framework_manifest=True)

        for service in registered_services:
            service_names = set(
                hal_service_name_utils.GetServiceNamesFromCompMatrix(
                    framework_comp_matrix_xml, service))
            service_instances[service] = service_names
        logging.info("registered service instances: %s", service_instances)

        service_instance_combinations = \
            hal_service_name_utils.GetServiceInstancesCombinations(
                registered_services, service_instances)

        return service_instance_combinations


if __name__ == "__main__":
    test_runner.main()
