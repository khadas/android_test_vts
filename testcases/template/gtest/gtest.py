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
import xml.etree.ElementTree

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.common import list_utils

from vts.testcases.template.gtest import gtest_test_case

DEVICE_TEST_DIR = '/data/local/tmp'
TAG_PATH_SEPARATOR = ':'


class Gtest(base_test_with_webdb.BaseTestWithWebDbClass):
    '''Bese class to run gtests.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        shell: ShellMirrorObject, shell mirror
        test_cases: list of GtestTestCase objects, list of test cases to run
        tags: all the tags that appeared in gtest binary list
    '''

    def setUpClass(self):
        '''Prepare class, push Gtest binary, and create test cases'''
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_GTEST_BINARY_PATHS,
        ]
        self.getUserParams(req_param_names=required_params)

        self.gtest_binary_paths = list_utils.ExpandItemDelimiters(
            self.gtest_binary_paths, const.LIST_ITEM_DELIMITER, strip=True)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)
        logging.info("%s: %s", keys.ConfigKeys.IKEY_GTEST_BINARY_PATHS,
                     self.gtest_binary_paths)

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self.shell = self._dut.shell.one
        self.testcases = []

        self.tags = set()
        # Push gtest binaries to device and create test cases
        for b in self.gtest_binary_paths:
            tag = ''
            path = b
            if TAG_PATH_SEPARATOR in b:
                tag, path = b.split(TAG_PATH_SEPARATOR)
            self.tags.add(tag)
            src = os.path.join(self.data_file_path, path)
            dst = self.GetDeviceSideBinaryPath(path)
            self._dut.adb.push("{src} {dst}".format(src=src, dst=dst))
            testcases = self.RetrieveTestCases(dst, tag)
            self.testcases.extend(testcases)

        self.include_filter = self.ExpandListItemTags(self.include_filter)
        self.exclude_filter = self.ExpandListItemTags(self.exclude_filter)

    def ExpandListItemTags(self, input_list):
        '''Expand list items with given tags

        Since gtest binary allows a tag to be added in front of the binary
        path, test names are generated with tags attached. This function is
        used to expand the filters correspondingly. If a filter contains
        a tag, only test name with that tag will be included in output.
        Otherwise, all known tags will be paired to the test name in output list

        Args:
            input_list: list of string, the list to expand

        Returns:
            A list of string
        '''
        result = []
        for item in input_list:
            if TAG_PATH_SEPARATOR in item:
                tag, name = item.split(TAG_PATH_SEPARATOR)
                result.append(gtest_test_case.PutTag(name, tag))
            for tag in self.tags:
                result.append(gtest_test_case.PutTag(item, tag))
        return result

    def tearDownClass(self):
        '''Perform clean-up jobs'''
        # Clean up the pushed gtest binary
        cmd = ['rm -rf {}'.format(
            self.GetDeviceSideBinaryPath(path.split(TAG_PATH_SEPARATOR)[-1]))
               for path in self.gtest_binary_paths]
        cmd.append('rm -rf %s' % self.GetDeviceSideBinaryPath(''))
        command_results = self.shell.Execute(cmd)

        if not command_results or any(command_results[const.EXIT_CODE]):
            logging.warning('Failed to clean up test class: %s',
                            command_results)

    def GetDeviceSideBinaryPath(self, path):
        '''Convert host side binary path to device side path.

        Args:
            path: string, host side gtest binary relative path

        Returns:
            string, device side gtest binary absolute path
        '''
        return os.path.join(DEVICE_TEST_DIR, 'gtest_binary_temp',
                            self.__class__.__name__, path)

    def RetrieveTestCases(self, path, tag=''):
        '''Create a list of TestCase objects from a list of test binary names.

        Args:
            path: string, absolute path of a gtest binary on device
            tag: string, a tag that will be appended to the end of test name

        Returns:
            A list of TestCase objects
        '''
        cmd_results = self.shell.Execute(
            'chmod 755 {path} && {path} --gtest_list_tests'.format(path=path))
        if cmd_results[const.EXIT_CODE][0]:
            logging.error('Failed to list test cases from binary %s' % path)

        result = []

        test_suite = ''
        for line in cmd_results[const.STDOUT][0].split('\n'):
            if not len(line.strip()):
                continue
            elif line.startswith(' '):  # Test case name
                test_name = line.strip()
                test_case = gtest_test_case.GtestTestCase(test_suite,
                                                          test_name, path, tag)
                logging.info('Gtest test case: %s' % test_case)
                result.append(test_case)
            else:  # Test suite name
                test_suite = line.strip()

        return result

    def VerifyGtestResults(self, command_results):
        '''Parse Gtest xml result output.

        Args:
            command_results: dict of lists, shell command result
        '''
        asserts.assertTrue(command_results, 'Empty command response.')
        asserts.assertEqual(len(command_results), 3, 'Empty command response.')
        asserts.assertFalse(
            any(command_results[const.EXIT_CODE]),
            'Some commands failed: %s' % command_results)

        xml_str = command_results[const.STDOUT][1].strip()
        root = xml.etree.ElementTree.fromstring(xml_str)
        asserts.assertEqual(root.get('tests'), '1', 'No tests available')
        asserts.assertEqual(
            root.get('errors'), '0', 'Error happened during test')
        asserts.assertEqual(
            root.get('failures'), '0', 'Gtest test case failed')
        asserts.skipIf(root.get('disabled') == '1', 'Gtest test case disabled')

    def RunGtestTestCase(self, test_case):
        '''Runs a gtest test_case.

        Args:
            test_case: GtestTestCase object
        '''
        cmd = [test_case.GetRunCommand(),
               'cat %s' % test_case.GetGtestOutputFileName(),
               'rm -rf %s' % test_case.GetGtestOutputFileName()]

        logging.info("Executing gtest command: %s", cmd)
        command_results = self.shell.Execute(cmd)

        self.VerifyGtestResults(command_results)

    def generateAllGtests(self):
        '''Runs all gtests.'''
        self.runGeneratedTests(
            test_func=self.RunGtestTestCase,
            settings=self.testcases,
            name_func=str)


if __name__ == "__main__":
    test_runner.main()
