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
from vts.utils.python.build.api import artifact_fetcher


class BuildProviderAB(build_provider.BuildProvider):
    """A build provider for Android Build (AB)."""

    def __init__(self):
        super(BuildProviderAB, self).__init__()
        if 'run_ab_key' in os.environ:
            print("For AB, use the key at %s" % os.environ['run_ab_key'])
            self._artifact_fetcher = artifact_fetcher.AndroidBuildClient(
                os.environ['run_ab_key'])
        else:
            self._artifact_fetcher = None

    def Fetch(self, branch, target, artifact_name, build_id="latest"):
        """Fetches Android device artifact file(s) from Android Build.

        Args:
            branch: string, androidbranch to pull resource from .
            target: string, build target name.
            artifact_name: string, file name.
            build_id: string, ID of the build or latest.

        Returns:
            a dict containing the device image info.
            a dict containing the test suite package info.
        """
        if not self._artifact_fetcher:
            return self.GetDeviceImage(), self.GetTestSuitePackage()

        if build_id == "latest":
            recent_build_ids = self._artifact_fetcher.ListBuildIds(
                branch, target)
            build_id = recent_build_ids[0]

        if "{build_id}" in artifact_name:
            artifact_name = artifact_name.replace("{build_id}", build_id)

        dest_filepath = os.path.join(self.tmp_dirpath, artifact_name)
        self._artifact_fetcher.DownloadArtifactToFile(
            branch, target, build_id, artifact_name,
            dest_filepath=dest_filepath)

        if dest_filepath.endswith("android-vts.zip"):
            self.SetTestSuitePackage("vts", dest_filepath)
        elif dest_filepath.endswith(".zip"):
            self.SetDeviceImageZip(dest_filepath)

        return self.GetDeviceImage(), self.GetTestSuitePackage()
