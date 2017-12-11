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

from vts.runners.host import utils

FULL_ZIPFILE = "full-zipfile"


class BuildProvider(object):
    """The base class for build provider.

    Attributes:
        _IMAGE_FILE_EXTENSIONS: a list of strings which are common image file
                                extensions.
        _BASIC_IMAGE_FILE_NAMES: a list of strings which are the image names in
                                 an artifact zip.
        _device_images: dict where the key is image file name and value is the
                        path.
        _test_suites: dict where the key is test suite type and value is the
                      test suite package file path.
        _tmp_dirpath: string, the temp dir path created to keep artifacts.
    """
    _IMAGE_FILE_EXTENSIONS = [".img", ".bin"]
    _BASIC_IMAGE_FILE_NAMES = ["boot.img", "system.img", "vendor.img"]

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

    def CreateNewTmpDir(self):
        return tempfile.mkdtemp(dir=self._tmp_dirpath)

    def SetDeviceImage(self, name, path):
        """Sets device image `path` for the specified `name`."""
        self._device_images[name] = path

    def _IsFullDeviceImage(self, namelist):
        """Returns true if given namelist list has all common device images."""
        for image_file in self._BASIC_IMAGE_FILE_NAMES:
            if image_file not in namelist:
                return False
        return True

    def _IsImageFile(self, file_path):
        """Returns whether a file is an image.

        Args:
            file_path: string, the file path.

        Returns:
            boolean, whether the file is an image.
        """
        return any(file_path.endswith(ext)
                   for ext in self._IMAGE_FILE_EXTENSIONS)

    def SetDeviceImagesInDirecotry(self, root_dir):
        """Sets device images to *.img and *.bin in a directory.

        Args:
            root_dir: string, the directory to find images in.
        """
        for dir_name, file_name in utils.iterate_files(root_dir):
            if self._IsImageFile(file_name):
                self.SetDeviceImage(file_name,
                                    os.path.join(dir_name, file_name))

    def SetDeviceImageZip(self, path):
        """Sets device image(s) using files in a given zip file.

        It extracts image files inside the given zip file and selects
        known Android image files.

        Args:
            path: string, the path to a zip file.
        """
        dest_path = path + ".dir"
        with zipfile.ZipFile(path, 'r') as zip_ref:
            if self._IsFullDeviceImage(zip_ref.namelist()):
                self.SetDeviceImage(FULL_ZIPFILE, path)
            else:
                zip_ref.extractall(dest_path)
                self.SetDeviceImagesInDirecotry(dest_path)

    def GetDeviceImage(self, name=None):
        """Returns device image info."""
        if name is None:
            return self._device_images
        return self._device_images[name]

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
        else:
            print("unsupported zip file %s" % path)
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
