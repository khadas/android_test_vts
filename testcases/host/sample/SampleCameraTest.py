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

from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class SampleCameraTest(base_test.BaseTestClass):
    """A sample testcase for the non-HIDL, conventional Camera HAL."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.hal.InitConventionalHal(target_type="camera",
                                         target_version=2.1,
                                         target_basepaths=["/data/local/tmp/32/hal"],
                                         bits=32)

    def TestCameraOpenFirst(self):
        """A simple testcase which just calls an open function."""
        self.dut.hal.camera.common.methods.open()  # note args are skipped

    def TestCameraInit(self):
        """A simple testcase which just calls an init function."""
        self.dut.hal.camera.init()  # expect an exception? (can be undefined)

    def testCameraNormal(self):
        """A simple testcase which just emulates a normal usage pattern."""
        result = self.dut.hal.camera.get_number_of_cameras()
        count = result.return_type.primitive_value[0].int32_t
        logging.info(count)
        for index in range(0, count):
            arg = self.dut.hal.camera.camera_info_t(facing=0)
            logging.info(self.dut.hal.camera.get_camera_info(index, arg))
        # uncomment when undefined function is handled gracefully.
        # self.dut.hal.camera.init()
        def camera_device_status_change():
            logging.info("camera_device_status_change")

        def torch_mode_status_change():
            logging.info("torch_mode_status_change")

        my_callback = self.dut.hal.camera.camera_module_callbacks_t(
            camera_device_status_change, torch_mode_status_change)
        self.dut.hal.camera.set_callbacks(my_callback)
        self.dut.hal.camera.common.methods.open()  # note args are skipped


if __name__ == "__main__":
    test_runner.main()
