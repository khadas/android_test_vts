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
        TAG_DELIMITER: string, separator used to separate tag and path
    '''

    DEVICE_TMP_DIR = '/data/local/tmp'
    TAG_DELIMITER = '::'
    PUSH_DELIMITER = '->'

    def setUpClass(self):
        '''Prepare class, push binaries, set permission, create test cases.'''
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_BINARY_TEST_SOURCES,
        ]
        opt_params = [keys.ConfigKeys.IKEY_BINARY_TEST_WORKING_DIRECTORIES,
                      keys.ConfigKeys.IKEY_BINARY_TEST_LD_LIBRARY_PATHS]
        self.getUserParams(
            req_param_names=required_params, opt_param_names=opt_params)

        self.binary_test_sources = list_utils.ExpandItemDelimiters(
            self.binary_test_sources,
            const.LIST_ITEM_DELIMITER,
            strip=True,
            to_str=True)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)
        logging.info("%s: %s", keys.ConfigKeys.IKEY_BINARY_TEST_SOURCES,
                     self.binary_test_sources)

        self.working_directories = {}
        if hasattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_WORKING_DIRECTORIES):
            self.binary_test_working_directories = list_utils.ExpandItemDelimiters(
                self.binary_test_working_directories,
                const.LIST_ITEM_DELIMITER,
                strip=True,
                to_str=True)
            for token in self.binary_test_working_directories:
                tag = ''
                path = token
                if self.TAG_DELIMITER in token:
                    tag, path = token.split(self.TAG_DELIMITER)
                self.working_directories[tag] = path

        self.ld_library_paths = {}
        if hasattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_LD_LIBRARY_PATHS):
            self.binary_test_ld_library_paths = list_utils.ExpandItemDelimiters(
                self.binary_test_ld_library_paths,
                const.LIST_ITEM_DELIMITER,
                strip=True,
                to_str=True)
            for token in self.binary_test_ld_library_paths:
                tag = ''
                path = token
                if self.TAG_DELIMITER in token:
                    tag, path = token.split(self.TAG_DELIMITER)
                if tag in self.ld_library_paths:
                    self.ld_library_paths[tag] = '{}:{}'.format(
                        self.ld_library_paths[tag], path)
                else:
                    self.ld_library_paths[tag] = path

        self._dut = self.registerController(android_device)[0]
        self._dut.shell.InvokeTerminal("one")
        self.shell = self._dut.shell.one

        self.testcases = []
        self.tags = set()
        self.CreateTestCases()
        cmd = list(
            set('chmod 755 %s' % test_case.path
                for test_case in self.testcases))
        cmd_results = self.shell.Execute(cmd)
        if any(cmd_results[const.EXIT_CODE]):
            logging.error('Failed to set permission to some of the binaries:\n'
                          '%s\n%s', cmd, cmd_results)

        self.include_filter = self.ExpandListItemTags(self.include_filter)
        self.exclude_filter = self.ExpandListItemTags(self.exclude_filter)

    def CreateTestCases(self):
        '''Push files to device and create test case objects.'''
        for source in self.binary_test_sources:
            logging.info('Parsing binary test source: %s', source)
            src, dst, tag = self.ParseTestSource(source)
            if src:
                if os.path.isdir(src):
                    src = os.path.join(src, '.')
                logging.info('Pushing from %s to %s.', src, dst)
                self._dut.adb.push("{src} {dst}".format(src=src, dst=dst))
            if tag is not None:
                # tag not being None means to create a test case
                self.tags.add(tag)
                logging.info('Creating test case from %s with tag %s', dst, tag)
                testcase = self.CreateTestCase(dst, tag)
                if not testcase:
                    continue

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
            if self.TAG_DELIMITER in item:
                tag, name = item.split(self.TAG_DELIMITER)
                result.append(self.PutTag(name, tag))
            for tag in self.tags:
                result.append(self.PutTag(item, tag))
        return result

    def tearDownClass(self):
        '''Perform clean-up jobs'''
        # Clean up the pushed binaries
        logging.info('Start class cleaning up jobs.')
        # Delete pushed files

        sources = set(
            self.ParseTestSource(src) for src in self.binary_test_sources)
        paths = [dst for src, dst, tag in sources if src and dst]
        cmd = ['rm -rf %s' % dst for dst in paths]
        cmd_results = self.shell.Execute(cmd)
        if not cmd_results or any(cmd_results[const.EXIT_CODE]):
            logging.warning('Failed to clean up test class: %s', cmd_results)

        # Delete empty directories in working directories
        dir_set = set(ntpath.dirname(dst) for dst in paths)
        dir_set.add(self.ParseTestSource('')[1])
        dirs = list(dir_set)
        dirs.sort(lambda x, y: cmp(len(y), len(x)))
        cmd = ['rmdir %s' % d for d in dirs]
        cmd_results = self.shell.Execute(cmd)
        if not cmd_results or any(cmd_results[const.EXIT_CODE]):
            logging.warning('Failed to remove: %s', cmd_results)

        logging.info('Finished class cleaning up jobs.')

    def ParseTestSource(self, source):
        '''Convert host side binary path to device side path.

        Args:
            source: string, binary test source string

        Returns:
            A tuple of (string, string, string), representing (host side
            absolute path, device side absolute path, tag). Returned tag
            will be None if the test source is for pushing file to working
            directory only.
        '''
        tag = ''
        path = source
        if self.TAG_DELIMITER in source:
            tag, path = source.split(self.TAG_DELIMITER)

        src = path
        dst = None
        if self.PUSH_DELIMITER in path:
            src, dst = path.split(self.PUSH_DELIMITER)

        if src:
            src = os.path.join(self.data_file_path, src)

        push_only = dst is not None and dst == ''

        if not dst:
            if tag in self.working_directories:
                dst = os.path.join(self.working_directories[tag],
                                   ntpath.basename(src))
            else:
                dst = os.path.join(self.DEVICE_TMP_DIR, 'binary_test_temp_%s' %
                                   self.__class__.__name__, tag,
                                   ntpath.basename(src))

        if push_only:
            tag = None

        return str(src), str(dst), tag

    def CreateTestCase(self, path, tag=''):
        '''Create a list of TestCase objects from a binary path.

        Args:
            path: string, absolute path of a binary on device
            tag: string, a tag that will be appended to the end of test name

        Returns:
            A list of BinaryTestCase objects
        '''
        working_directory = self.working_directories[
            tag] if tag in self.working_directories else None
        ld_library_path = self.ld_library_paths[
            tag] if tag in self.ld_library_paths else None
        return binary_test_case.BinaryTestCase(
            '', ntpath.basename(path), path, tag, self.PutTag,
            working_directory, ld_library_path)

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
