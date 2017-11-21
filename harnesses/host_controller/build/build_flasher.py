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
"""Class to flash build artifacts onto devices"""

import os

from vts.utils.python.controllers import android_device


class BuildFlasher(object):
    """Client that manages build flashing.

    Attributes:
        device: AndroidDevice, the device associated with the client.
    """

    def __init__(self, serial=""):
        """Initialize the client.

        If serial not provided, find single device connected. Error if
        zero or > 1 devices connected.

        Args:
            serial: optional string, serial number for the device.
        """
        if serial != "":
            self.device = android_device.AndroidDevice(serial)
        else:
            serials = android_device.list_adb_devices()
            if len(serials) == 0:
                raise android_device.AndroidDeviceError(
                    "ADB could not find any target devices.")
            if len(serials) > 1:
                raise android_device.AndroidDeviceError(
                    "ADB found more than one device: %s" % serials)
            self.device = android_device.AndroidDevice(serials[0])

    def FlashGSI(self, system_img, vbmeta_img=None):
        """Flash the Generic System Image to the device.

        Args:
            system_img: string, path to GSI
            vbmeta_img: string, optional, path to vbmeta image for new devices
        """
        if not os.path.exists(system_img):
            raise ValueError("Couldn't find system image at %s" % system_img)
        self.device.adb.wait_for_device()
        if not self.device.isBootloaderMode:
            self.device.log.info(self.device.adb.reboot_bootloader())
        if vbmeta_img is not None:
            self.device.fastboot.flash('vbmeta', vbmeta_img)
        self.device.log.info(self.device.fastboot.erase('system'))
        self.device.log.info(self.device.fastboot.flash('system', system_img))
        self.device.log.info(self.device.fastboot.erase('metadata'))
        self.device.log.info(self.device.fastboot._w())
        self.device.log.info(self.device.fastboot.reboot())

    def Flashall(self, directory):
        """Flash all images in a directory to the device using flashall.

        Generally the directory is the result of unzipping the .zip from AB.
        Args:
            directory: string, path to directory containing images
        """
        # fastboot flashall looks for imgs in $ANDROID_PRODUCT_OUT
        os.environ['ANDROID_PRODUCT_OUT'] = directory
        self.device.adb.wait_for_device()
        if not self.device.isBootloaderMode:
            self.device.log.info(self.device.adb.reboot_bootloader())
        self.device.log.info(self.device.fastboot.flashall())
