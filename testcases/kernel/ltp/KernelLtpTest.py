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
from vts.runners.host import const


class TestCase(object):
    """Stores name, path, and param information for each test case.

    Attributes:
        _testsuite: string, name of testsuite to which the testcase belongs
        _testname: string, name of the test case
        _testbinary: string, test binary path
        _args: list of string, test case command line arguments
    """

    def __init__(self, testsuite, testname, testbinary, args):
        self._testsuite = testsuite
        self._testname = testname
        self._testbinary = testbinary
        self._args = args

    @property
    def testsuite(self):
        """Get the test suite's name."""
        return self._testsuite

    @property
    def testname(self):
        """Get the test case's name."""
        return self._testname

    @property
    def testbinary(self):
        """Get the test binary's file name."""
        return self._testbinary

    def GetArgs(self, replace_string_from=None, replace_string_to=None):
        """Get the case arguments in string format.

        if replacement string is provided, arguments will be
        filtered with the replacement string

        Args:
            replace_string_from: string, replacement string source, if any
            replace_string_to: string, replacement string destination, if any

        Returns:
            string, arguments formated in space separated string
        """
        if replace_string_from is None or replace_string_to is None:
            return ' '.join(self._args)
        else:
            return ' '.join([p.replace(replace_string_from, replace_string_to)
                             for p in self._args])


class TestcaseParser(object):
    """Load a ltp vts testcase definition file and parse it into a generator.

    Attributes:
        _data_path: string, the vts data path on host side
    """

    def __init__(self, data_path):
        self._data_path = data_path

    def _GetTestcaseFilePath(self):
        """return the test case definition fiile's path."""
        return os.path.join(self._data_path, '32', 'ltp',
                            'ltp_vts_testcases.txt')

    def Load(self):
        """read the definition file and return a TestCase generator."""
        with open(self._GetTestcaseFilePath(), 'r') as f:
            for line in f:
                items = line.split('\t')
                if not len(items) == 4:
                    continue
                testsuite, testname, testbinary, arg = items

                if testname.startswith("DISABLED_"):
                    continue

                args = []
                if arg:
                    args.extend(arg.split(','))
                yield TestCase(testsuite, testname, testbinary, args)


class TempDir(object):
    """Class used to prepare and clean temporary directory.

    Attributes:
        _path: string, path to the temp dir
        _executor: ShellMirrorObject,
                   shell mirror object used to execute commands
    """

    def __init__(self, path, executor):
        self._path = path
        self._executor = executor

    def Prepare(self):
        """Prepare a temporary directory."""
        # TODO: check error
        self._executor.Execute(["mkdir %s -p" % self._path,
                                "chmod 775 %s" % self._path,
                                "mkdir %s/tmp" % self._path,
                                "chmod 775 %s/tmp" % self._path])

    def Clean(self):
        """Clean a temporary directory."""
        # TODO: check error
        self._executor.Execute("rm -rf %s" % self._path)

    @property
    def path(self):
        """Get the temporary directory path."""
        return self._path


class KernelLtpTest(base_test.BaseTestClass):
    """Runs the LTP (Linux Test Project) testcases against Android OS kernel.

    Attributes:
        _TPASS: int, exit_code for Test pass
        _TCONF: int, The test case is not for current configuration of kernel
        _32BIT: int, for 32 bit tests
        _64BIT: int, for 64 bit tests
        _dut: AndroidDevice, the device under test
        _shell: ShellMirrorObject, shell mirror object used to execute commands
        _ltp_dir: string, ltp build root directory on target
        _temp_dir: TempDir, temporary directory manager
        _testcases: TestcaseParser, test case input parser
        _env: dict<stirng, stirng>, dict of environment variable key value pair
        _KEY_ENV__: constant strings starting with prefix "_KEY_ENV_" are used as dict
                    key in environment variable dictionary
    """
    _TPASS = 0
    _TCONF = 32
    _32BIT = 32
    _64BIT = 64
    _KEY_ENV_TMPDIR = 'TMPDIR'
    _KEY_ENV_TMP = 'TMP'
    _KEY_ENV_LTP_DEV_FS_TYPE = 'LTP_DEV_FS_TYPE'
    _KEY_ENV_LTPROOT = 'LTPROOT'
    _KEY_ENV_PATH = 'PATH'

    def setUpTest(self):
        """Creates a remote shell instance, and copies data files."""
        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self._shell = self._dut.shell.one
        self._ltp_dir = "/data/local/tmp/ltp"

        self._temp_dir = TempDir("/data/local/tmp/ltp/temp", self._shell)

        self._testcases = TestcaseParser(self.data_file_path)
        self._env = {self._KEY_ENV_TMPDIR: self._temp_dir._path,
                     self._KEY_ENV_TMP: "%s/tmp" % self._temp_dir._path,
                     self._KEY_ENV_LTP_DEV_FS_TYPE: "ext4",
                     self._KEY_ENV_LTPROOT: self._ltp_dir,
                     self._KEY_ENV_PATH:
                     "/system/bin:%s/testcases/bin" % self._ltp_dir, }

    def PushFiles(self, n_bit):
        """Push the related files to target.

        Args:
            n_bit: int, _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """

        self._shell.Execute(["rm -rf  %s" % self._ltp_dir,
                             "mkdir %s -p" % self._ltp_dir])
        self._dut.adb.push("%s/%i/ltp/. %s" %
                           (self.data_file_path, n_bit, self._ltp_dir))
        # TODO: libcap

    def GetEnvp(self):
        """Generate the environment variable required to run the tests."""
        return ' '.join("%s=%s" % (key, value)
                        for key, value in self._env.items())

    def tearDownTest(self):
        """Deletes all copied data files."""
        self._shell.Execute("rm -rf %s" % self._ltp_dir)

    def Verify(self, testcase_name, results):
        """Verifies the test result of each test case."""

        if len(results) == 0:
            # TIMEOUT
            logging.info("[Test Case] %s FAIL" % testcase_name)
            return

        logging.info("stdout: %s" % str(results[const.STDOUT]))
        logging.info("stderr: %s" % str(results[const.STDERR]))
        logging.info("exit_code: %s" %
                     str(results[const.EXIT_CODE]))

        # For LTP test cases, we run one shell command for each test case
        # So the result should also contains only one execution output
        if results[const.EXIT_CODE][0] not in (self._TPASS, self._TCONF) or \
                   "TFAIL" in results[const.STDOUT][0]:
            # If return code is other than 0 or 32,
            # or the output contains 'TFAIL', then test FAIL
            logging.info("[Test Case] %s FAIL" % testcase_name)
        elif results[const.EXIT_CODE][0] == self._TPASS:
            # If output exit_code is _TPASS, then test PASS
            logging.info("[Test Case] %s PASS" % testcase_name)
        elif results[const.EXIT_CODE][0] == self._TCONF and \
            "TPASS" in results[const.STDOUT][0]:
            # If output exit_code is _TCONF or but the stdout
            # contains TPASS, this means part of the test passed
            # but others were skipped. We consider it a PASS
            logging.info("[Test Case] %s PASS" % testcase_name)
        elif results[const.EXIT_CODE][0] == self._TCONF:
            # Test case is not for the current configuration, SKIP
            # TODO: emit SKIP message
            logging.info("[Test Case] %s FAIL" % testcase_name)
        else:
            # All other cases are treated as FAIL, but this is not expected
            logging.info("[Test Case] %s FAIL" % testcase_name)

    def TestNBits(self, n_bit):
        """Runs all 32-bit or 64-bit LTP test cases.

        Args:
            n_bit: _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """
        self.PushFiles(n_bit)
        logging.info("[Test Case] test%iBits PASS" % n_bit)

        for test_case in self._testcases.Load():
            self._temp_dir.Prepare()
            testcase_name = "%s-%s_%ibit" % (test_case._testsuite,
                                             test_case._testname, n_bit)
            logging.info("[Test Case] %s" % testcase_name)
            path = os.path.join(self._ltp_dir, "testcases/bin",
                                test_case._testbinary)

            logging.info("executing a test binary: [%s]" % path)
            logging.info("execution envp: [%s]" % self.GetEnvp())
            logging.info("execution params: [%s]" % \
                         test_case.GetArgs("$LTPROOT", self._ltp_dir))

            self._dut.shell.InvokeTerminal(testcase_name)
            shell = getattr(self._dut.shell, testcase_name)

            shell.Execute("chmod 775 " + path)

            results = shell.Execute("env %s %s %s" % (
                self.GetEnvp(), path, test_case.GetArgs("$LTPROOT",
                                                        self._ltp_dir)))

            self.Verify(testcase_name, results)

            self._temp_dir.Clean()

    def test32Bits(self):
        """Runs all 32-bit LTP test cases."""
        self.TestNBits(self._32BIT)

#     def test64Bits(self):
#         # TODO: enable 64 bit tests
#         """Runs all 64-bit LTP test cases."""
#         self.TestNBits(self._64BIT)

if __name__ == "__main__":
    test_runner.main()
