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
from vts.runners.host import const
from vts.runners.host import test_runner

from vts.testcases.template.binary_test import binary_test
from vts.testcases.template.gtest_binary_test import gtest_test_case


class GtestBinaryTest(binary_test.BinaryTest):
    '''Base class to run gtests binary on target.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        shell: ShellMirrorObject, shell mirror
        test_cases: list of GtestTestCase objects, list of test cases to run
        tags: all the tags that appeared in binary list
        DEVICE_TEST_DIR: string, temp location for storing binary
        TAG_PATH_SEPARATOR: string, separator used to separate tag and path
    '''

    # @Override
    def CreateTestCaseFromBinary(self, path, tag=''):
        '''Create a list of GtestTestCase objects from a binary path.

        Args:
            path: string, absolute path of a gtest binary on device
            tag: string, a tag that will be appended to the end of test name

        Returns:
            A list of GtestTestCase objects
        '''
        cmd_results = self.shell.Execute(
            'chmod 755 {path} && {path} --gtest_list_tests'.format(path=path))
        if cmd_results[const.EXIT_CODE][0]:
            logging.error('Failed to list test cases from binary %s' % path)

        test_cases = []

        test_suite = ''
        for line in cmd_results[const.STDOUT][0].split('\n'):
            if not len(line.strip()):
                continue
            elif line.startswith(' '):  # Test case name
                test_name = line.strip()
                test_case = gtest_test_case.GtestTestCase(test_suite,
                                                          test_name, path, tag)
                logging.info('Gtest test case: %s' % test_case)
                test_cases.append(test_case)
            else:  # Test suite name
                test_suite = line.strip()
                if test_suite.endswith('.'):
                    test_suite = test_suite[:-1]

        return test_cases

    # @Override
    def VerifyTestResult(self, test_case, command_results):
        '''Parse Gtest xml result output.

        Args:
            test_case: GtestTestCase object, the test being run. This param
                       is not currently used in this method.
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


if __name__ == "__main__":
    test_runner.main()
