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
import os

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class KernelLtpTest(base_test.BaseTestClass):
    """Runs the LTP (Linux Test Project) testcases against Android OS kernel."""

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        self.dut = self.registerController(android_device)[0]
        self.dut.shell.InvokeTerminal("one")
        self.shell = self.dut.shell.one

        self.ltp_dir = "/data/local/tmp/ltp"
        # Copy LTP test case files.
        self.shell.Execute("mkdir %s -p" % self.ltp_dir)
        self.shell.Execute("mkdir %s/32 -p" % self.ltp_dir)
        self.dut.adb.push("%s/32/ltp/testcases/bin/* %s/32" %
                          (self.data_file_path, self.ltp_dir))
        self.shell.Execute("mkdir %s/64 -p" % self.ltp_dir)
        self.dut.adb.push("%s/64/ltp/testcases/bin/* %s/64" %
                          (self.data_file_path, self.ltp_dir))

    def tearDownClass(self):
        """Deletes all copied data files."""
        self.shell.Execute("rm -rf /data/local/tmp/ltp")

    def Verify(self, binary, all_stdout):
        """Verifies the test result of each test case."""
        logging.info("stdout: %s" % all_stdout)
        if "TPASS" in all_stdout and "TFAIL" not in all_stdout:
            logging.info("[Test Case] %s PASS" % binary)
        else:
            logging.info("[Test Case] %s FAIL" % binary)

    def test32Bits(self):
        """Runs all 32-bit LTP test cases."""
        logging.info("[Test Case] test32Bits PASS")
        stdouts = self.shell.Execute("ls %s/32" % self.ltp_dir)
        for binary in stdouts[0].split():
            testcase_name = "%s_32bit" % binary
            logging.info("[Test Case] %s" % testcase_name)
            path = os.path.join(self.ltp_dir, "32", binary)
            logging.info("executing a command '%s'" % path)
            self.dut.shell.InvokeTerminal(testcase_name)
            shell = getattr(self.dut.shell, testcase_name)
            shell.Execute("chmod 755 " + path)
            stdouts = shell.Execute("env TMPDIR=/data/local/tmp " + path)
            self.Verify(testcase_name, "\n".join(stdouts))

    def test64Bits(self):
        """Runs all 64-bit LTP test cases."""
        logging.info("[Test Case] Test64Bits PASS")
        stdouts = self.shell.Execute("ls %s/64" % self.ltp_dir)
        for binary in stdouts[0].split():
            testcase_name = "%s_64bit" % binary
            logging.info("[Test Case] %s" % testcase_name)
            path = os.path.join(self.ltp_dir, "64", binary)
            logging.info("executing a command '%s'" % path)
            self.dut.shell.InvokeTerminal(testcase_name)
            shell = getattr(self.dut.shell, testcase_name)
            shell.Execute("chmod 755 " + path)
            stdouts = shell.Execute("env TMPDIR=/data/local/tmp " + path)
            self.Verify(testcase_name, "\n".join(stdouts))


if __name__ == "__main__":
    test_runner.main()
