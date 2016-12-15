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

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.runners.host import const


class LibcTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """A basic test of the libc API."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.lib.InitSharedLib(target_type="bionic_libc",
                                   target_basepaths=["/system/lib64"],
                                   target_version=1.0,
                                   target_filename="libc.so",
                                   bits=64,
                                   handler_name="libc")

    def testFileIO(self):
        """Tests open, read, and close file IO operations."""
        logging.info("Test result: %s",
            self.dut.lib.libc.fopen("/system/lib64/libc.so", "r"))


if __name__ == "__main__":
    test_runner.main()
