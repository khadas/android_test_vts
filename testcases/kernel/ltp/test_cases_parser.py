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

import os
import logging
import itertools

from vts.testcases.kernel.ltp import ltp_configs
from vts.testcases.kernel.ltp import ltp_enums
from vts.testcases.kernel.ltp import test_case


class TestCasesParser(object):
    """Load a ltp vts testcase definition file and parse it into a generator.

    Attributes:
        _data_path: string, the vts data path on host side
    """

    def __init__(self, data_path, include_filter, exclude_filter):
        self._data_path = data_path
        self._include_filter = self.ExpandFilters(include_filter)
        self._exclude_filter = self.ExpandFilters(exclude_filter)

    def ExpandFilters(self, filter):
        """Expend comma delimited filter list to a full list of test names.

        This is a workaround for developers' convenience to specify multiple
        test names within one filter command.

        Arguments:
            filter: list of strings, items in the list that are comma delimited
                    will be expanded

        Returns:
            A expanded list of strings contains test names in filter.
        """
        expended_list_generator = (
            item.split(ltp_enums.Delimiters.TESTCASE_FILTER)
            for item in filter)
        return list(itertools.chain.from_iterable(expended_list_generator))

    def IsTestcaseInList(self, testcase, name_list, n_bit):
        """Check whether a given testcase object is in a disabled test list"""
        name_set = (testcase.fullname, testcase.testname,
                    "{test_name}_{n_bit}bit".format(
                        test_name=testcase.fullname, n_bit=n_bit))
        return len(set(name_set).intersection(name_list)) > 0

    def ValidateDefinition(self, line):
        """Validate a tab delimited test case definition.

        Will check whether the given line of definition has three parts
        separated by tabs.
        It will also trim leading and ending white spaces for each part
        in returned tuple (if valid).

        Returns:
            A tuple in format (test suite, test name, test command) if
            definition is valid. None otherwise.
        """
        items = [
            item.strip()
            for item in line.split(ltp_enums.Delimiters.TESTCASE_DEFINITION)
        ]
        if not len(items) == 3 or not items:
            return None
        else:
            return items

    def Load(self, ltp_dir, n_bit, run_staging=False):
        """Read the definition file and yields a TestCase generator."""
        run_scritp = self.GenerateLtpRunScript()

        for line in run_scritp:
            items = self.ValidateDefinition(line)
            if not items:
                continue

            testsuite, testname, command = items

            # Tests failed to build will have prefix "DISABLED_"
            if testname.startswith("DISABLED_"):
                logging.info("[Parser] Skipping test case {}-{}. Reason: "
                             "not built".format(testsuite, testname))
                continue

            # Some test cases contain semicolons in their commands,
            # and we replace them with &&
            command = command.replace(';', '&&')

            testcase = test_case.TestCase(
                testsuite=testsuite, testname=testname, command=command)

            # If include filter is set from TradeFed command, then
            # yield only the included tests, regardless whether the test
            # is disabled in configuration
            if self._include_filter:
                if self.IsTestcaseInList(testcase, self._include_filter,
                                         n_bit):
                    logging.info("[Parser] Adding test case %s. Reason: "
                                 "include filter" % testcase.fullname)
                    yield testcase
                continue

            # Check exclude filter set from TradeFed command. If a test
            # case is in both include and exclude filter, it will be
            # included
            if self.IsTestcaseInList(testcase, self._exclude_filter, n_bit):
                logging.info("[Parser] Skipping test case %s. Reason: "
                             "exclude filter" % testcase.fullname)
                continue

            # For skipping tests that are not designed for Android
            if self.IsTestcaseInList(testcase, ltp_configs.DISABLED_TESTS,
                                     n_bit):
                logging.info("[Parser] Skipping test case %s. Reason: "
                             "disabled" % testcase.fullname)
                continue

            # For failing tests that are being inspected
            if (not run_staging and self.IsTestcaseInList(
                    testcase, ltp_configs.STAGING_TESTS, n_bit)):
                logging.info("[Parser] Skipping test case %s. Reason: "
                             "staging" % testcase.fullname)
                continue

            logging.info("[Parser] Adding test case %s." % testcase.fullname)
            yield testcase

    def ReadCommentedTxt(self, filepath):
        '''Read a lines of a file that are not commented by #.

        Args:
            filepath: string, path of file to read

        Returns:
            A set of string representing non-commented lines in given file
        '''
        if not filepath:
            logging.error('Invalid file path')
            return None

        with open(filepath, 'r') as f:
            lines_gen = (line.strip() for line in f)
            return set(
                line for line in lines_gen
                if line and not line.startswith('#'))

    def GenerateLtpTestCases(self, testsuite, disabled_tests_list):
        '''Generate test cases for each ltp test suite.

        Args:
            testsuite: string, test suite name

        Returns:
            A list of string
        '''
        testsuite_script = os.path.join(self._data_path,
                                        ltp_configs.LTP_RUNTEST_DIR, testsuite)
        testsuite = testsuite.replace('-', '_')

        result = []
        for line in open(testsuite_script, 'r'):
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            testname = line.split()[0]
            testname_prefix = ('DISABLED_'
                               if testname in disabled_tests_list else '')
            testname_modified = testname_prefix + testname.replace('-', '_')

            result.append("\t".join([testsuite, testname_modified, line[len(
                testname):].strip()]))
        return result

    def GenerateLtpRunScript(self):
        '''Given a scenario group generate test case script.

        Args:
            scenario_group: string, file path of scanerio group file

        Returns:
            A list of string
        '''
        disabled_tests_path = os.path.join(
            self._data_path, ltp_configs.LTP_DISABLED_BUILD_TESTS_CONFIG_PATH)
        disabled_tests_list = self.ReadCommentedTxt(disabled_tests_path)

        result = []
        for testsuite in ltp_configs.TEST_SUITES:
            result.extend(
                self.GenerateLtpTestCases(testsuite, disabled_tests_list))

        #TODO(yuexima): remove duplicate

        return result
