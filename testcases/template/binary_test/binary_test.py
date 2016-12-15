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
import ntpath

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.common import list_utils

from vts.testcases.template.binary_test import binary_test_case


class BinaryTest(base_test_with_webdb.BaseTestWithWebDbClass):
    '''Base class to run binary tests on target.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        shell: ShellMirrorObject, shell mirror
        test_cases: list of BinaryTestCase objects, list of test cases to run
        tags: all the tags that appeared in binary list
        DEVICE_TMP_DIR: string, temp location for storing binary
        TAG_PATH_DELIMITER: string, separator used to separate tag and path
    '''

    DEVICE_TMP_DIR = '/data/local/tmp'
    TAG_PATH_DELIMITER = ':'

    def setUpClass(self):
        '''Prepare class, push binaries, set permission, create test cases.'''
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_BINARY_TEST_SOURCES,
        ]
        self.getUserParams(req_param_names=required_params)

        self.binary_test_sources = map(lambda s: '{}'.format(s),
                                       self.binary_test_sources)
        self.binary_test_sources = list_utils.ExpandItemDelimiters(
            self.binary_test_sources, const.LIST_ITEM_DELIMITER, strip=True)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)
        logging.info("%s: %s", keys.ConfigKeys.IKEY_BINARY_TEST_SOURCES,
                     self.binary_test_sources)

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self.shell = self._dut.shell.one

        self.testcases = []
        self.tags = set()
        self.CreateTestCases()

        cmd = ['chmod 755 %s' % test_case.path for test_case in self.testcases]
        cmd_results = self.shell.Execute(cmd)
        if any(cmd_results[const.EXIT_CODE]):
            logging.error('Failed to set permission to some of the binaries:\n'
                          '%s\n%s', cmd, cmd_results)

        self.include_filter = self.ExpandListItemTags(self.include_filter)
        self.exclude_filter = self.ExpandListItemTags(self.exclude_filter)

    def CreateTestCases(self):
        '''Push files to device and create test case objects.'''
        for b in self.binary_test_sources:
            tag = ''
            path = b
            if self.TAG_PATH_DELIMITER in b:
                tag, path = b.split(self.TAG_PATH_DELIMITER)
            self.tags.add(tag)
            src = os.path.join(self.data_file_path, path)
            dst = self.GetDeviceSidePath(path, tag)
            self._dut.adb.push("{src} {dst}".format(src=src, dst=dst))
            testcase = self.CreateTestCaseFromBinary(dst, tag)
            if type(testcase) is list:
                self.testcases.extend(testcase)
            else:
                self.testcases.append(testcase)

    def PutTag(self, name, tag):
        '''Put tag on name and return the resulting string.

        Args:
            name: string, a test name
            tag: string

        Returns:
            String, the result string after putting tag on the name
        '''
        return '{}{}'.format(name, tag)

    def ExpandListItemTags(self, input_list):
        '''Expand list items with tags.

        Since binary test allows a tag to be added in front of the binary
        path, test names are generated with tags attached. This function is
        used to expand the filters correspondingly. If a filter contains
        a tag, only test name with that tag will be included in output.
        Otherwise, all known tags will be paired to the test name in output
        list.

        Args:
            input_list: list of string, the list to expand

        Returns:
            A list of string
        '''
        result = []
        for item in input_list:
            if self.TAG_PATH_DELIMITER in item:
                tag, name = item.split(self.TAG_PATH_DELIMITER)
                result.append(self.PutTag(name, tag))
            for tag in self.tags:
                result.append(self.PutTag(item, tag))
        return result

    def tearDownClass(self):
        '''Perform clean-up jobs'''
        # Clean up the pushed binaries
        logging.info('Start class cleaning up jobs.')
        command_results = self.shell.Execute('rm -rf %s' %
                                             self.GetDeviceSidePath(''))

        if not command_results or any(command_results[const.EXIT_CODE]):
            logging.warning('Failed to clean up test class: %s',
                            command_results)
        logging.info('Finished class cleaning up jobs.')

    def GetDeviceSidePath(self, path, tag=''):
        '''Convert host side binary path to device side path.

        Args:
            path: string, host side binary relative path

        Returns:
            string, device side binary absolute path
        '''
        return os.path.join(self.DEVICE_TMP_DIR,
                            'binary_test_temp_%s' % self.__class__.__name__,
                            tag, ntpath.basename(path))

    def CreateTestCaseFromBinary(self, path, tag=''):
        '''Create a list of TestCase objects from a binary path.

        Args:
            path: string, absolute path of a binary on device
            tag: string, a tag that will be appended to the end of test name

        Returns:
            A list of BinaryTestCase objects
        '''
        return binary_test_case.BinaryTestCase('', ntpath.basename(path), path,
                                               tag, self.PutTag)

    def VerifyTestResult(self, test_case, command_results):
        '''Parse command result.

        Args:
            command_results: dict of lists, shell command result
        '''
        asserts.assertTrue(command_results, 'Empty command response.')
        asserts.assertFalse(
            any(command_results[const.EXIT_CODE]),
            'Test {} failed with the following results: {}'.format(
                test_case, command_results))

    def RunTestCase(self, test_case):
        '''Runs a test_case.

        Args:
            test_case: BinaryTestCase object
        '''
        cmd = test_case.GetRunCommand()
        logging.info("Executing binary test command: %s", cmd)
        command_results = self.shell.Execute(cmd)

        self.VerifyTestResult(test_case, command_results)

    def generateAllTests(self):
        '''Runs all binary tests.'''
        self.runGeneratedTests(
            test_func=self.RunTestCase, settings=self.testcases, name_func=str)


if __name__ == "__main__":
    test_runner.main()
