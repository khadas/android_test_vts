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

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.file import target_file_utils


class VtsCodelabHidlHandleTest(base_test.BaseTestClass):
    """This class tests hidl_handle API calls from host side.

    Attributes:
        TEST_DIR_PATH: string, directory where the test file will be created.
        TEST_FILE_PATH: string, path to the file that host side uses.
        _created: bool, whether we created TEST_DIR_PATH.
        _writer: ResourceHidlHandleMirror, writer who writes to TEST_FILE_PATH.
        _reader: ResourceHidlHandleMirror, reader who reads from TEST_FILE_PATH.
    """

    TEST_DIR_PATH = "/data/local/tmp/vts_codelab_tmp/"
    TEST_FILE_PATH = TEST_DIR_PATH + "test.txt"

    def setUpClass(self):
        """Necessary setup for the test environment.

        Load a light HAL driver, and create a /tmp directory to create
        test files in it.
        """
        self.dut = self.android_devices[0]
        # Initialize a hal driver to start all managers on the target side,
        # not used for other purposes.
        self.dut.hal.InitHidlHal(
            target_type="light",
            target_basepaths=self.dut.libPaths,
            target_version_major=2,
            target_version_minor=0,
            target_package="android.hardware.light",
            target_component_name="ILight",
            bits=int(self.abi_bitness))

        # Check if a tmp directory under /data exists.
        self._created = False
        if not target_file_utils.Exists(self.TEST_DIR_PATH, self.dut.shell):
            # Create a tmp directory under /data.
            self.dut.shell.Execute("mkdir " + self.TEST_DIR_PATH)
            # Verify it succeeds. Stop test if it fails.
            if not target_file_utils.Exists(self.TEST_DIR_PATH,
                                            self.dut.shell):
                logging.error("Failed to create " + self.TEST_DIR_PATH +
                              " directory. Stopping test.")
                return False
            # Successfully created the directory.
            logging.info("Manually created " + self.TEST_DIR_PATH +
                         " for the test.")
            self._created = True

    def setUp(self):
        """Initialize a writer and a reader for each test case.

        We open the file with w+ in every test case, which will create the file
        if it doesn't exist, or truncate the file if it exists, because some
        test case won't fully read the content of the file, causing dependency
        between test cases.
        """
        self._writer = self.dut.resource.InitHidlHandleForSingleFile(
            self.TEST_FILE_PATH,
            "w+",
            client=self.dut.hal.GetTcpClient("light"))
        self._reader = self.dut.resource.InitHidlHandleForSingleFile(
            self.TEST_FILE_PATH,
            "r",
            client=self.dut.hal.GetTcpClient("light"))
        asserts.assertTrue(self._writer is not None,
                           "Writer should be initialized successfully.")
        asserts.assertTrue(self._reader is not None,
                           "Reader should be initialized successfully.")
        asserts.assertNotEqual(self._writer.handleId, -1)
        asserts.assertNotEqual(self._reader.handleId, -1)

    def tearDown(self):
        """Cleanup for each test case.

        This method closes all open file descriptors on target side.
        """
        # Close all file descriptors by calling CleanUp().
        self.dut.resource.CleanUp()

    def tearDownClass(self):
        """Cleanup after all tests.

        Remove self.TEST_FILE_DIR directory if we created it at the beginning.
        """
        # Delete self.TEST_DIR_PATH if we created it.
        if self._created:
            logging.info("Deleting " + self.TEST_DIR_PATH +
                         " directory created for this test.")
            self.dut.shell.Execute("rm -rf " + self.TEST_DIR_PATH)

    def testInvalidWrite(self):
        """Test writing to a file with no write permission should fail. """
        read_data = self._reader.writeFile("abc", 3)
        asserts.assertTrue(
            read_data is None,
            "Reader should fail because it doesn't have write permission to the file."
        )

    def testOpenNonexistingFile(self):
        """Test opening a nonexisting file with read-only flag should fail.

        This test case first checks if the file exists. If it exists, it skips
        this test case. If it doesn't, it will try to open the non-existing file
        with 'r' flag.
        """
        if not target_file_utils.Exists(self.TEST_DIR_PATH + "abc.txt",
                                        self.dut.shell):
            logging.info("Test opening a non-existing file with 'r' flag.")
            failed_reader = self.dut.resource.InitHidlHandleForSingleFile(
                self.TEST_DIR_PATH + "abc.txt",
                "r",
                client=self.dut.hal.GetTcpClient("light"))
            asserts.assertTrue(
                failed_reader is None,
                "Open a non-existing file with 'r' flag should fail.")

    def testSimpleReadWrite(self):
        """Test a simple read/write interaction between reader and writer. """
        write_data = "Hello World!"
        asserts.assertEqual(
            len(write_data), self._writer.writeFile(write_data,
                                                    len(write_data)))
        read_data = self._reader.readFile(len(write_data))
        asserts.assertEqual(write_data, read_data)

    def testLargeReadWrite(self):
        """Test consecutive reads/writes between reader and writer. """
        write_data = "Android VTS"

        for i in range(10):
            asserts.assertEqual(
                len(write_data),
                self._writer.writeFile(write_data, len(write_data)))
            curr_read_data = self._reader.readFile(len(write_data))
            asserts.assertEqual(curr_read_data, write_data)


if __name__ == "__main__":
    test_runner.main()
