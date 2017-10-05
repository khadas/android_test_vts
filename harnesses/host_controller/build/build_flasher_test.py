#!/usr/bin/env python
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

import unittest
from vts.harnesses.host_controller.build import build_flasher

try:
    from unittest import mock
except ImportError:
    import mock


class BuildFlasherTest(unittest.TestCase):
    """Tests for Build Flasher"""

    @mock.patch(
        "vts.harnesses.host_controller.build.build_flasher.android_device")
    @mock.patch("vts.harnesses.host_controller.build.build_flasher.os")
    def testFlashGSIBadPath(self, mock_os, mock_class):
        flasher = build_flasher.BuildFlasher("thisismyserial")
        mock_os.path.exists.return_value = False
        with self.assertRaises(ValueError) as cm:
            flasher.FlashGSI("notexists.img")
        self.assertEqual("Couldn't find system image at notexists.img",
                         str(cm.exception))

    @mock.patch(
        "vts.harnesses.host_controller.build.build_flasher.android_device")
    @mock.patch("vts.harnesses.host_controller.build.build_flasher.os")
    def testFlashGSISystemOnly(self, mock_os, mock_class):
        mock_device = mock.Mock()
        mock_class.AndroidDevice.return_value = mock_device
        flasher = build_flasher.BuildFlasher("thisismyserial")
        mock_os.path.exists.return_value = True
        flasher.FlashGSI("exists.img")
        mock_device.fastboot.erase.assert_any_call('system')
        mock_device.fastboot.flash.assert_any_call('system', 'exists.img')
        mock_device.fastboot.erase.assert_any_call('metadata')

    @mock.patch(
        "vts.harnesses.host_controller.build.build_flasher.android_device")
    def testFlashall(self, mock_class):
        mock_device = mock.Mock()
        mock_class.AndroidDevice.return_value = mock_device
        flasher = build_flasher.BuildFlasher("thisismyserial")
        flasher.Flashall("path/to/dir")
        mock_device.fastboot.flashall.assert_called_with()

    @mock.patch(
        "vts.harnesses.host_controller.build.build_flasher.android_device")
    def testEmptySerial(self, mock_class):
        mock_class.list_adb_devices.return_value = ['oneserial']
        flasher = build_flasher.BuildFlasher(serial="")
        mock_class.AndroidDevice.assert_called_with("oneserial")


if __name__ == "__main__":
    unittest.main()
