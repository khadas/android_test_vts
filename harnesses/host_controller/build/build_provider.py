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

import shutil
import tempfile


class BuildProvider(object):
    """The base class for build provider.

    Attributes:
        IMAGE_FILE_NAMES: a list of strings where each string contains the known
                          Android device image file name.
        _device_images: dict where the key is image type and value is the
                        image file path.
        _test_suites: dict where the key is test suite type and value is the
                      test suite package file path.
        _tmp_dirpath: string, the temp dir path created to keep artifacts.
    """
    IMAGE_FILE_NAMES = ["boot.img", "system.img", "vendor.img", "userdata.img",
                        "vbmeta.img", "metadata.img"]

    def __init__(self):
        self._device_images = {}
        self._test_suites = {}
        self._tmp_dirpath = tempfile.mkdtemp()

    def __del__(self):
        """Deletes the temp dir if still set."""
        if self._tmp_dirpath:
            shutil.rmtree(self._tmp_dirpath)
            self._tmp_dirpath = None

    @property
    def tmp_dirpath(self):
        return self._tmp_dirpath

    def SetDeviceImage(self, type, path):
        """Sets device image `path` for the specified `type`."""
        self._device_images[type] = path

    def GetDeviceImage(self, type=None):
        """Returns device image info."""
        if type is None:
            return self._device_images
        return self._device_images[type]

    def SetTestSuitePackage(self, type, path):
        """Sets test suite package `path` for the specified `type`."""
        self._test_suites[type] = path

    def GetTestSuitePackage(self, type=None):
        """Returns test suite package info."""
        if type is None:
            return self._test_suites
        return self._test_suites[type]

    def PrintDeviceImageInfo(self):
        """Prints device image info."""
        print("%s" % self.GetDeviceImage())

    def PrintGetTestSuitePackageInfo(self):
        """Prints test suite package info."""
        print("%s" % self.GetTestSuitePackage())
