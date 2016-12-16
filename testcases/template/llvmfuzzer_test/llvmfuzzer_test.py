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
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.common import list_utils

from vts.testcases.template.llvmfuzzer_test import llvmfuzzer_test_config as config

class LLVMFuzzerTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Runs fuzzer tests on target.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        _shell: ShellMirrorObject, shell mirror
        _testcases: string list, list of testcases to run
        _test_flags: string, flags that will be passed to fuzzer
    """
    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_BINARY_TEST_SOURCES
        ]
        self.getUserParams(required_params)

        self._testcases = list_utils.ExpandItemDelimiters(
            self.binary_test_sources,
            const.LIST_ITEM_DELIMITER,
            strip=True,
            to_str=True)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            self.data_file_path)
        logging.info("%s: %s", keys.ConfigKeys.IKEY_BINARY_TEST_SOURCES,
            self._testcases)

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self._shell = self._dut.shell.one
        self._test_flags = " ".join(["-%s=%s" % (k, v) for k, v in config.FUZZER_PARAMS.items()])
        self._shell.Execute("mkdir %s -p" % config.FUZZER_TEST_DIR)

    def tearDownClass(self):
        """Deletes all copied data."""
        self._shell.Execute("rm -rf %s" % config.FUZZER_TEST_DIR)

    def PushFiles(self, testcase):
        """adb pushes testcase file to target.

        Args:
            testcase: string, path to executable fuzzer.
        """
        push_src = os.path.join(self.data_file_path, testcase)
        self._dut.adb.push("%s %s" % (push_src, config.FUZZER_TEST_DIR))
        logging.info("Adb pushed: %s", testcase)

    def RunTestcase(self, testcase):
        """Runs the given testcase and asserts the result.

        Args:
            testcase: string, path to fuzzer executable.
        """
        self.PushFiles(testcase)
        fuzzer = testcase.split("/")[-1]

        chmod_cmd = "chmod -R 755 %s" % os.path.join(config.FUZZER_TEST_DIR, fuzzer)
        cd_cmd = "cd %s" % config.FUZZER_TEST_DIR
        ld_path = "LD_LIBRARY_PATH=/data/local/tmp/32:/data/local/tmp/64:$LD_LIBRARY_PATH"
        test_cmd = "./%s" % fuzzer

        cmd = [
            chmod_cmd,
            "%s && %s %s %s" % (cd_cmd, ld_path, test_cmd, self._test_flags)
        ]
        logging.info("Executing: %s", cmd)

        result = self._shell.Execute(cmd)
        self.AssertTestResult(fuzzer, result)

    def LogCrashReport(self, fuzzer):
        """Logs crash-causing fuzzer input.

        Reads the crash report file and logs the contents in format:
        "\x01\x23\x45\x67\x89\xab\xcd\xef"

        Args:
            fuzzer: string, name of fuzzer executable.
        """
        cmd = "xxd -p %s" % config.FUZZER_TEST_CRASH_REPORT

        # output is string of a hexdump from crash report file.
        # From the example above, output would be "0123456789abcdef".
        output = self._shell.Execute(cmd)[const.STDOUT][0]
        remove_chars = ["\r", "\t", "\n", " "]
        for char in remove_chars:
            output = output.replace(char, "")

        crash_report = ""
        # output is guaranteed to be even in length since its a hexdump.
        for offset in xrange(0, len(output), 2):
            crash_report += "\\x%s" % output[offset:offset + 2]

        logging.info('FUZZER_TEST_CRASH_REPORT for %s: "%s"', fuzzer, crash_report)

    # TODO(trong): differentiate between crashes and sanitizer rule violations.
    def AssertTestResult(self, fuzzer, result):
        """Asserts that testcase finished as expected.

        Checks that device is in responsive state. If not, waits for boot
        then reports test as failure. If it is, asserts that all test commands
        returned exit code 0.

        Args:
            fuzzer: string, name of fuzzer executable.
            result: dict([str],[str],[int]), command results from shell.
        """
        if not self._dut.hasBooted():
            self._dut.waitForBootCompletion()
            asserts.fail("%s left the device in unresponsive state." % fuzzer)

        # Last exit code is the exit code of the fuzzer executable.
        exit_code = result[const.EXIT_CODE][-1]
        if exit_code == config.ExitCode.FUZZER_TEST_FAIL:
            self.LogCrashReport(fuzzer)
            asserts.fail("%s failed normally." % fuzzer)
        elif exit_code != config.ExitCode.FUZZER_TEST_PASS:
            asserts.fail("%s failed abnormally." % fuzzer)

    def generateFuzzerTests(self):
        """Runs fuzzer tests."""
        self.runGeneratedTests(
            test_func=self.RunTestcase,
            settings=self._testcases,
            name_func=lambda x: x.split("/")[-1])


if __name__ == "__main__":
    test_runner.main()
