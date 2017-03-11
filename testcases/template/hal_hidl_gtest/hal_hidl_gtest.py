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

from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.testcases.template.gtest_binary_test import gtest_binary_test
from vts.utils.python.cpu import cpu_frequency_scaling
from xml.etree import ElementTree

_HWBINDER = "hwbinder"
_NAME = "name"
_PASSTHROUGH = "passthrough"
_TRANSPORT = "transport"
_VERSION = "version"


class HidlHalGTest(gtest_binary_test.GtestBinaryTest):
    '''Base class to run a VTS target-side HIDL HAL test.

    Attributes:
        DEVICE_TEST_DIR: string, temp location for storing binary
        TAG_PATH_SEPARATOR: string, separator used to separate tag and path
        shell: ShellMirrorObject, shell mirror
        tags: all the tags that appeared in binary list
        testcases: list of GtestTestCase objects, list of test cases to run
        _cpu_freq: CpuFrequencyScalingController instance of a target device.
        _dut: AndroidDevice, the device under test as config
        _skip_all_testcases: boolean - to skip all test cases. set when a target
                             device does not have a required HIDL service.
    '''

    def setUpClass(self):
        """Turns on CPU frequency scaling."""
        super(HidlHalGTest, self).setUpClass()

        opt_params = [
            keys.ConfigKeys.IKEY_PRECONDITION_HWBINDER_SERVICE,
            keys.ConfigKeys.IKEY_PRECONDITION_FEATURE,
            keys.ConfigKeys.IKEY_PRECONDITION_FILE_PATH_PREFIX,
            keys.ConfigKeys.IKEY_PRECONDITION_LSHAL,
            keys.ConfigKeys.IKEY_SKIP_IF_THERMAL_THROTTLING
        ]
        self.getUserParams(opt_param_names=opt_params)

        passthrough_opt = self.getUserParam(
                keys.ConfigKeys.IKEY_PASSTHROUGH_MODE, default_value=False)
        self._skip_if_thermal_throttling = self.getUserParam(
                keys.ConfigKeys.IKEY_SKIP_IF_THERMAL_THROTTLING,
                default_value=False)

        # Enable coverage if specified in the configuration or coverage enabled.
        # TODO(ryanjcampbell@) support binderized mode
        if passthrough_opt or self.coverage.enabled:
            self._EnablePassthroughMode()

        self._cpu_freq = None
        self._skip_all_testcases = False

        hwbinder_service_name = str(
            getattr(self, keys.ConfigKeys.IKEY_PRECONDITION_HWBINDER_SERVICE,
                    ""))
        if hwbinder_service_name:
            if not hwbinder_service_name.startswith("android.hardware."):
                logging.error("The given hwbinder service name %s is invalid.",
                              hwbinder_service_name)
            else:
                cmd_results = self.shell.Execute("ps -A")
                hwbinder_service_name += "@"
                if (any(cmd_results[const.EXIT_CODE]) or hwbinder_service_name
                        not in cmd_results[const.STDOUT][0]):
                    logging.warn("The required hwbinder service %s not found.",
                                 hwbinder_service_name)
                    self._skip_all_testcases = True

        if not self._skip_all_testcases:
            feature = str(
                getattr(self, keys.ConfigKeys.IKEY_PRECONDITION_FEATURE, ""))
            if feature:
                if not feature.startswith("android.hardware."):
                    logging.error(
                        "The given feature name %s is invalid for HIDL HAL.",
                        feature)
                else:
                    cmd_results = self.shell.Execute("pm list features")
                    if (any(cmd_results[const.EXIT_CODE]) or
                            feature not in cmd_results[const.STDOUT][0]):
                        logging.warn("The required feature %s not found.",
                                     feature)
                        self._skip_all_testcases = True

        if not self._skip_all_testcases:
            file_path_prefix = str(
                getattr(self, keys.ConfigKeys.
                        IKEY_PRECONDITION_FILE_PATH_PREFIX, ""))
            if file_path_prefix:
                cmd_results = self.shell.Execute("ls %s*" % file_path_prefix)
                if any(cmd_results[const.EXIT_CODE]):
                    logging.warn("The required file (prefix: %s) not found.",
                                 file_path_prefix)
                    self._skip_all_testcases = True

        if not self._skip_all_testcases:
            feature = str(
                getattr(self, keys.ConfigKeys.IKEY_PRECONDITION_LSHAL, ""))
            if feature:
                cmd_results = self.shell.Execute(
                    "lshal --init-vintf 2> /dev/null")
                if cmd_results[const.EXIT_CODE][0] == 0:
                    if cmd_results[const.STDOUT][0]:
                        try:
                            hwbinder_hals, passthrough_hals = self.ParseVintfXml(
                                cmd_results[const.STDOUT][0])
                            if (feature not in hwbinder_hals and
                                feature not in passthrough_hals):
                                logging.warn("The required feature %s not found.",
                                             feature)
                                self._skip_all_testcases = True
                        except ElementTree.ParseError as e:
                            logging.exception(e)
                            logging.error("can't check precondition due to a "
                                          "lshal output format error: %s",
                                          cmd_results[const.STDOUT][0])
        if not self._skip_all_testcases:
            self._cpu_freq = cpu_frequency_scaling.CpuFrequencyScalingController(
                self._dut)
            self._cpu_freq.DisableCpuScaling()

    @staticmethod
    def ParseVintfXml(vintf_xml):
      """Parses a vintf xml string.

      Args:
          vintf_xml: string, containing a vintf.xml file content.

      Returns:
          a list containing the package names of hwbinder HALs,
          a list containing the package names of passthrough HALs.
      """
      xml_root = ElementTree.fromstring(vintf_xml)

      hwbinder_hals = []
      passthrough_hals = []

      for xml_hal in xml_root:
          hal_name = None
          hal_transport = None
          hal_version = None
          for xml_hal_item in xml_hal:
              if str(xml_hal_item.tag) == _NAME:
                  hal_name = str(xml_hal_item.text)
              elif str(xml_hal_item.tag) == _TRANSPORT:
                  hal_transport = str(xml_hal_item.text)
              elif str(xml_hal_item.tag) == _VERSION:
                  hal_version = str(xml_hal_item.text)
          hal_package_name = "%s@%s" % (hal_name, hal_version)
          if hal_transport == _HWBINDER:
              hwbinder_hals.append(hal_package_name)
          elif hal_transport == _PASSTHROUGH:
              passthrough_hals.append(hal_package_name)
          else:
              logging.error("Unknown transport type %s", hal_transport)
      return hwbinder_hals, passthrough_hals

    def _EnablePassthroughMode(self):
        """Enable passthrough mode by setting getStub to true.

        This funciton should be called after super class' setupClass method.
        If called before setupClass, user_params will be changed in order to
        trigger setupClass method to invoke this method again.
        """
        if self.testcases:
            for test_case in self.testcases:
                envp = ' %s=true' % const.VTS_HAL_HIDL_GET_STUB
                test_case.envp += envp
        else:
            logging.warn('No test cases are defined yet. Maybe setupClass '
                         'has not been called. Changing user_params to '
                         'enable passthrough mode option.')
            self.user_params[keys.ConfigKeys.IKEY_PASSTHROUGH_MODE] = True

    def setUpTest(self):
        """Skips the test case if thermal throttling lasts for 30 seconds."""
        super(HidlHalGTest, self).setUpTest()
        if not self._skip_all_testcases:
            if self._cpu_freq and self._skip_if_thermal_throttling:
                self._cpu_freq.SkipIfThermalThrottling(retry_delay_secs=30)
        else:
            logging.info("Skip a test case.")

    def tearDownTest(self):
        """Skips the test case if there is thermal throttling."""
        if not self._skip_all_testcases:
            if self._cpu_freq and self._skip_if_thermal_throttling:
                self._cpu_freq.SkipIfThermalThrottling()

        super(HidlHalGTest, self).tearDownTest()

    def tearDownClass(self):
        """Turns off CPU frequency scaling."""
        if not self._skip_all_testcases:
            if self._cpu_freq:
                self._cpu_freq.EnableCpuScaling()

        super(HidlHalGTest, self).tearDownClass()


if __name__ == "__main__":
    test_runner.main()
