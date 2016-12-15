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
from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.runners.host import const


class ShellBinaryCrashTest(base_test.BaseTestClass):
    """A binary crash test case for the shell driver."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]

    def testCrashBinary(self):
        """Tests whether the agent survives when a called binary crashes."""
        self.dut.shell.InvokeTerminal("my_shell1")
        target = "/data/local/tmp/64/vts_test_binary_crash_app"
        results = self.dut.shell.my_shell1.Execute(
            ["chmod 755 %s" % target,
             ".%s" % target])
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 2)
        asserts.assertEqual(results[const.STDOUT][1].strip(), "")
        # "crash_app: start"
        asserts.assertEqual(results[const.EXIT_CODE][1], 127)

        results = self.dut.shell.my_shell1.Execute("which ls")
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 1)
        asserts.assertEqual(results[const.STDOUT][0].strip(),
                            "/system/bin/ls")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)

        self.dut.shell.InvokeTerminal("my_shell2")
        results = self.dut.shell.my_shell2.Execute("which ls")
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 1)
        asserts.assertEqual(results[const.STDOUT][0].strip(),
                            "/system/bin/ls")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)


if __name__ == "__main__":
    test_runner.main()
