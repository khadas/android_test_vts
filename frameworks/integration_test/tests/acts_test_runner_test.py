#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.


import shutil
import tempfile
import unittest

from acts import keys
from acts import signals
from acts import test_runner
import mock_controller


class ActsTestRunnerTest(unittest.TestCase):
    """This test class has unit tests for the implementation of everything
    under acts.test_runner.
    """

    def setUp(self):
        self.tmp_dir = tempfile.mkdtemp()
        self.base_mock_test_config = {
            "testbed":{
                "name": "SampleTestBed"
            },
            "logpath": self.tmp_dir,
            "cli_args": None,
            "testpaths": ["../tests/sample"]
        }
        self.mock_run_list = [('SampleTest', None)]

    def tearDown(self):
        shutil.rmtree(self.tmp_dir)

    def test_register_controller_no_config(self):
        tr = test_runner.TestRunner(self.base_mock_test_config,
                                    self.mock_run_list)
        with self.assertRaisesRegexp(signals.ControllerError,
                                     "No corresponding config found for"):
            tr.register_controller(mock_controller)

    def test_register_controller_dup_register(self):
        """Verifies correctness of internal tally of controller objects and the
        right error happen when a controller module is registered twice.
        """
        mock_test_config = dict(self.base_mock_test_config)
        tb_key = keys.Config.key_testbed.value
        mock_ctrlr_config_name = mock_controller.ACTS_CONTROLLER_CONFIG_NAME
        mock_ctrlr_ref_name = mock_controller.ACTS_CONTROLLER_REFERENCE_NAME
        mock_test_config[tb_key][mock_ctrlr_config_name] = ["magic1", "magic2"]
        tr = test_runner.TestRunner(self.base_mock_test_config,
                                    self.mock_run_list)
        tr.register_controller(mock_controller)
        mock_ctrlrs = tr.test_run_info[mock_ctrlr_ref_name]
        self.assertEqual(mock_ctrlrs[0].magic, "magic1")
        self.assertEqual(mock_ctrlrs[1].magic, "magic2")
        self.assertTrue(tr.controller_destructors[mock_ctrlr_ref_name])
        expected_msg = "Controller module .* has already been registered."
        with self.assertRaisesRegexp(signals.ControllerError, expected_msg):
            tr.register_controller(mock_controller)

    def test_register_controller_return_value(self):
        mock_test_config = dict(self.base_mock_test_config)
        tb_key = keys.Config.key_testbed.value
        mock_ctrlr_config_name = mock_controller.ACTS_CONTROLLER_CONFIG_NAME
        mock_ctrlr_ref_name = mock_controller.ACTS_CONTROLLER_REFERENCE_NAME
        mock_test_config[tb_key][mock_ctrlr_config_name] = ["magic1", "magic2"]
        tr = test_runner.TestRunner(self.base_mock_test_config,
                                    self.mock_run_list)
        magic_devices = tr.register_controller(mock_controller)
        self.assertEqual(magic_devices[0].magic, "magic1")
        self.assertEqual(magic_devices[1].magic, "magic2")

    def test_verify_controller_module(self):
        test_runner.TestRunner.verify_controller_module(mock_controller)

    def test_verify_controller_module_null_attr(self):
        try:
            tmp = mock_controller.ACTS_CONTROLLER_CONFIG_NAME
            mock_controller.ACTS_CONTROLLER_CONFIG_NAME = None
            msg = "Controller interface .* in .* cannot be null."
            with self.assertRaisesRegexp(signals.ControllerError, msg):
                test_runner.TestRunner.verify_controller_module(mock_controller)
        finally:
            mock_controller.ACTS_CONTROLLER_CONFIG_NAME = tmp

    def test_verify_controller_module_missing_attr(self):
        try:
            tmp = mock_controller.ACTS_CONTROLLER_CONFIG_NAME
            delattr(mock_controller, "ACTS_CONTROLLER_CONFIG_NAME")
            msg = "Module .* missing required controller module attribute"
            with self.assertRaisesRegexp(signals.ControllerError, msg):
                test_runner.TestRunner.verify_controller_module(mock_controller)
        finally:
            setattr(mock_controller, "ACTS_CONTROLLER_CONFIG_NAME", tmp)


if __name__ == "__main__":
   unittest.main()