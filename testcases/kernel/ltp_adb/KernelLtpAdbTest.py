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
        _nbits: string, '32' or '64' number of bits used to represent an
                integer in a target library.
    """

    def __init__(self, data_path, nbits):
        self._data_path = data_path
        self._nbits = nbits

    def _GetTestcaseFilePath(self):
        """return the test case definition fiile's path."""
        return os.path.join(self._data_path, self._nbits, 'ltp',
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
        _adb: AdbProxy instance, to send adb commands
    """

    def __init__(self, path, adb):
        self._path = path
        self._adb = adb

    def Prepare(self):
        """Prepare a temporary directory."""
        # TODO: check error
        logging.info("TempDir: Prepare %s", self._path)
        self._adb.shell("mkdir -p %s" % self._path)
        self._adb.shell("chmod 775 %s" % self._path)
        self._adb.shell("mkdir -p %s/tmp" % self._path)
        self._adb.shell("chmod 775 %s/tmp" % self._path)

    def Clean(self):
        """Clean a temporary directory."""
        # TODO: check error
        logging.info("TempDir: Clean %s", self._path)
        self._adb.shell("rm -rf %s" % self._path)

    @property
    def path(self):
        """Get the temporary directory path."""
        return self._path


class KernelLtpAdbTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Runs the LTP (Linux Test Project) testcases against Android OS kernel.

    Attributes:
        _TPASS: int, exit_code for Test pass
        _TCONF: int, The test case is not for current configuration of kernel
        _32BIT: int, for 32 bit tests
        _64BIT: int, for 64 bit tests
        _dut: AndroidDevice, the device under test
        _adb: AdbProxy instance, to send adb commands
        _ltp_dir: string, ltp build root directory on target
        _temp_dir: TempDir, temporary directory manager
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

    def setUpClass(self):
        """Prepares an AdbProxy instance, and copies data files."""
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)

        logging.info("data_file_path: %s", self.data_file_path)
        self._dut = self.registerController(android_device)[0]
        self._adb = self._dut.adb
        self._ltp_dir = "/data/local/tmp/ltp"

        self._temp_dir = TempDir("/data/local/tmp/ltp/temp", self._adb)
        self._temp_dir.Prepare()

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
        logging.info("PushFiles")
        self._adb.shell("mkdir %s -p" % self._ltp_dir)
        self._dut.adb.push("%s/%i/ltp/. %s" %
                           (self.data_file_path, n_bit, self._ltp_dir))

    def GetEnvp(self):
        """Generate the environment variable required to run the tests."""
        return ' '.join("%s=%s" % (key, value)
                        for key, value in self._env.items())

    def tearDownClass(self):
        """Deletes all copied data files."""
        logging.info("tearDownClass")
        self._adb.shell("rm -rf %s" % self._ltp_dir)
        self._temp_dir.Clean()

    def Verify(self, results):
        """Verifies the test result of each test case."""
        if not results:
            # TIMEOUT
            asserts.fail("No response received. Socket timeout")

        logging.info("stdout: %s" % results)
        if "TFAIL" in results or "TPASS" not in results:
            asserts.fail("Test failed")

    def TestNBits(self, n_bit):
        """Runs all 32-bit or 64-bit LTP test cases.

        Args:
            n_bit: _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """
        testcases = TestcaseParser(self.data_file_path, str(n_bit))
        self.PushFiles(n_bit)
        logging.info("[Test Case] test%iBits SKIP" % n_bit)

        self.runGeneratedTests(test_func=self.RunLtpOnce,
                               settings=testcases.Load(),
                               args=(n_bit,),
                               name_func=self.GetTestName)

        logging.info("[Test Case] test%iBits" % n_bit)
        raise asserts.skip("Finished generating {} bit tests.".format(n_bit))

    def GetTestName(self, test_case, n_bit):
        "Generate the vts test name of a ltp test"
        return "%s-%s_%ibit" % (test_case._testsuite,
                                test_case._testname, n_bit)

    def RunLtpOnce(self, test_case, n_bit):
        "Run one LTP test case"
        testcase_name = self.GetTestName(test_case, n_bit)

        path = os.path.join(self._ltp_dir, "testcases/bin",
                            test_case._testbinary)

        logging.info("executing a test binary: [%s]" % path)
        logging.info("execution envp: [%s]" % self.GetEnvp())
        logging.info("execution params: [%s]" % \
                     test_case.GetArgs("$LTPROOT", self._ltp_dir))

        self._adb.shell("chmod 775 " + path)

        cmd = "env %s LD_LIBRARY_PATH=%s:/data/local/tmp/%s/:$LD_LIBRARY_PATH %s %s" % (
            self.GetEnvp(),
            self._ltp_dir,
            n_bit,
            path,
            test_case.GetArgs("$LTPROOT", self._ltp_dir))
        logging.info("cmd: %s", cmd)
        results = self._adb.shell(cmd)

        self.Verify(results)

    def test32Bits(self):
        """Runs all 32-bit LTP test cases."""
        self.TestNBits(self._32BIT)

#     def test64Bits(self):
#         # TODO: enable 64 bit tests
#         """Runs all 64-bit LTP test cases."""
#         self.TestNBits(self._64BIT)

if __name__ == "__main__":
    test_runner.main()
