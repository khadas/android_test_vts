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

import os
import shutil
import tempfile
import zipfile

FULL_ZIPFILE = "full-zipfile"


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
    IMAGE_FILE_NAMES = ["boot.img", "cache.img", "metadata.img", "modem.img",
                        "keymaster.img", "system.img", "userdata.img",
                        "vbmeta.img", "vendor.img"]
    BASIC_IMAGE_FILE_NAMES = ["boot.img", "system.img", "vendor.img"]

    def __init__(self):
        self._device_images = {}
        self._test_suites = {}
        tempdir_base = os.path.join(os.getcwd(), "tmp")
        if not os.path.exists(tempdir_base):
            os.mkdir(tempdir_base)
        self._tmp_dirpath = tempfile.mkdtemp(dir=tempdir_base)

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

    def IsFullDeviceImage(self, namelist):
        """Returns true if given namelist list has all common device images."""
        for image_file in self.BASIC_IMAGE_FILE_NAMES:
            if image_file not in namelist:
                return False
        return True

    def SetDeviceImageZip(self, path):
        """Sets device image(s) using files in a given zip file.

        It extracts image files inside the given zip file and selects
        known Android image files using self.IMAGE_FILE_NAMES.

        Args:
            path: string, the path to a zip file.
        """
        dest_path = path + ".dir"
        with zipfile.ZipFile(path, 'r') as zip_ref:
            if self.IsFullDeviceImage(zip_ref.namelist()):
                self.SetDeviceImage(FULL_ZIPFILE, path)
            else:
                zip_ref.extractall(dest_path)
                artifact_paths = map(
                    lambda filename: os.path.join(dest_path, filename) if (
                        filename and (filename.endswith(".img")
                                      or filename.endswith(".zip"))) else None,
                    zip_ref.namelist())
                artifact_paths = filter(None, artifact_paths)
                if artifact_paths:
                    for artifact_path in artifact_paths:
                        for known_filename in self.IMAGE_FILE_NAMES:
                            if artifact_path.endswith(known_filename):
                                self.SetDeviceImage(
                                    known_filename.replace(".img", ""),
                                    artifact_path)

    def GetDeviceImage(self, type=None):
        """Returns device image info."""
        if type is None:
            return self._device_images
        return self._device_images[type]

    def SetTestSuitePackage(self, type, path):
        """Sets test suite package `path` for the specified `type`.

        Args:
            type: string, test suite type such as 'vts' or 'cts'.
            path: string, the path of a file. if a file is a zip file,
                  it's unziped and its main binary is set.
        """
        if path.endswith("android-vts.zip"):
            dest_path = os.path.join(self.tmp_dirpath, "android-vts")
            with zipfile.ZipFile(path, 'r') as zip_ref:
                zip_ref.extractall(dest_path)
                bin_path = os.path.join(dest_path, "android-vts",
                                        "tools", "vts-tradefed")
                os.chmod(bin_path, 0766)
                path = bin_path
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
