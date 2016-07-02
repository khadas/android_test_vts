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


class SampleShellTest(base_test.BaseTestClass):
    """A sample testcase for the shell driver."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("my_shell1")

    def testOneCommand(self):
        """A simple testcase which just emulates a normal usage pattern."""
        stdouts = self.dut.shell.my_shell1.Execute("which ls")
        logging.info(stdouts)
        for stdout in stdouts:
            asserts.assertEqual(stdout.strip(), "/system/bin/ls")


if __name__ == "__main__":
    test_runner.main()
