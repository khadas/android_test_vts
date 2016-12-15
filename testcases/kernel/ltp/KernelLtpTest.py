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


class KernelLtpTest(base_test.BaseTestClass):
    """Runs the LTP (Linux Test Project) testcases against Android OS kernel."""

    def setUpClass(self):
        """Creates a remote shell instance."""
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self.shell = self.dut.shell.one

    def Verify(self, binary, all_stdout):
        """Verifies the test result of each test case."""
        logging.info("stdout: %s" % all_stdout)
        asserts.assertTrue("TPASS" in all_stdout and "TFAIL" not in all_stdout,
                           "command [%s] failed" % binary)

    def testCrashTestcases32Bits(self):
        """Crash01 and Crash02 test cases (32-bit mode)."""
        for binary in ["/data/local/tmp/32/crash01_32",
                       "/data/local/tmp/32/crash02_32"]:
            self.shell.Execute("chmod 755 " + binary)
            logging.info("executing a command '%s'" % binary)
            stdouts = self.shell.Execute("env TMPDIR=/data/local/tmp " + binary)
            self.Verify(binary, "\n".join(stdouts))


    def testCrashTestcases64Bits(self):
        """Crash01 and Crash02 test cases (64-bit mode)."""
        for binary in ["/data/local/tmp/64/crash01_64",
                       "/data/local/tmp/64/crash02_64"]:
            self.shell.Execute("chmod 755 " + binary)
            logging.info("executing a command '%s'" % binary)
            stdouts = self.shell.Execute(
                "env TMPDIR=/data/local/tmp LD_LIBRARY_PATH=/data/local/tmp/64/ " + binary)
            self.Verify(binary, "\n".join(stdouts))


if __name__ == "__main__":
    test_runner.main()
