#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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
"""VTS test to verify userspace fastboot implementation."""

import os
import subprocess

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner

PROPERTY_LOGICAL_PARTITIONS = "ro.boot.dynamic_partitions"


class VtsFastbootVerificationTest(base_test.BaseTestClass):
    """Verifies userspace fastboot implementation."""

    def setUpClass(self):
        """Initializes the DUT and places devices into fastboot mode."""
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell
        if self.dut.getProp(PROPERTY_LOGICAL_PARTITIONS) != "true":
            self.skipAllTests("Device does not support userspace fastboot")
        else:
            self.dut.cleanUp()
            self.shell.Execute("reboot fastboot")

    def testFastbootdImplementation(self):
        """Runs fuzzy_fastboot to verify fastboot implementation."""
        fastboot_gtest_bin_path = os.path.join(
            "host", "nativetest64", "fuzzy_fastboot", "fuzzy_fastboot")
        fastboot_gtest_cmd = [
            "%s" % fastboot_gtest_bin_path, "--gtest_filter=Conformance*"
        ]
        #TODO(b/117181762): Add a serial number argument to fuzzy_fastboot.
        retcode = subprocess.call(fastboot_gtest_cmd)
        asserts.assertTrue(retcode == 0, "Fastboot implementation has errors")

    def tearDownClass(self):
        """Reboot to Android."""
        if self.isSkipAllTests():
            return
        if self.dut.isBootloaderMode:
            self.dut.reboot()
            self.dut.waitForBootCompletion()


if __name__ == "__main__":
    test_runner.main()
