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
import zipfile

from vts.harnesses.host_controller.build import build_provider


class BuildProviderLocalFS(build_provider.BuildProvider):
    """A build provider for local file system (fs)."""

    def __init__(self):
        super(BuildProviderLocalFS, self).__init__()

    def Fetch(self, path):
        """Fetches Android device artifact file(s) from a local directory.

        Args:
            path: string, the path of a directory which keeps artifacts.

        Returns:
            a dict containing the device image info.
            a dict containing the test suite package info.
        """
        if os.path.isdir(path):
            # Consider checking the existences of those files before setting
            # and raising an error if a required file is missing.
            for image_type in ["boot", "system", "vendor", "userdata"]:
                file_path = os.path.join(path, "%s.img" % image_type)
                if os.path.isfile(file_path):
                    self.SetDeviceImage(image_type, file_path)
                else:
                    print("%s image file doesn't exist" % image_type)
        else:
            if path.endswith("android-vts.zip"):
                if os.path.isfile(path):
                    dest_path = os.path.join(self.tmp_dirpath, "android-vts")
                    with zipfile.ZipFile(path, 'r') as zip_ref:
                        zip_ref.extractall(dest_path)
                        bin_path = os.path.join(dest_path, "android-vts",
                                                "tools", "vts-tradefed")
                        os.chmod(bin_path, 0766)
                        self.SetTestSuitePackage("vts", bin_path)
                else:
                    print("The specified file doesn't exist, %s" % path)
            else:
                print("unsupported zip file %s" % path)
        return self.GetDeviceImage(), self.GetTestSuitePackage()
