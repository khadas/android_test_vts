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

from vts.testcases.kernel.ltp import ltp_configs
from vts.testcases.kernel.ltp import test_case


class TestCasesParser(object):
    """Load a ltp vts testcase definition file and parse it into a generator.

    Attributes:
        _data_path: string, the vts data path on host side
    """

    def __init__(self, data_path):
        self._data_path = data_path

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
        items = [item.strip() for item in line.split('\t')]
        if not len(items) == 3 or not items:
            return None
        else:
            return items

    def Load(self, ltp_dir, n_bit, run_staging=False):
        """Read the definition file and yields a TestCase generator."""
        case_definition_path = os.path.join(self._data_path, n_bit, 'ltp',
                                            'ltp_vts_testcases.txt')

        with open(case_definition_path, 'r') as f:
            for line in f:
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

                # For skipping tests that are not designed for Android
                if self.IsTestcaseInList(testcase, ltp_configs.DISABLED_TESTS,
                                         n_bit):
                    logging.info("[Parser] Skipping test case {}-{}. Reason: "
                                 "disabled".format(testsuite, testname))
                    continue

                # For failing tests that are being inspected
                if (not run_staging and self.IsTestcaseInList(
                        testcase, ltp_configs.STAGING_TESTS, n_bit)):
                    logging.info("[Parser] Skipping test case {}-{}. Reason: "
                                 "staging".format(testsuite, testname))
                    continue

                yield testcase
