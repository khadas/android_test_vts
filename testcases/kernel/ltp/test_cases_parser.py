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

    def Load(self, ltp_dir, n_bit, run_staging=False):
        """Read the definition file and return a TestCase generator."""
        case_definition_path = os.path.join(self._data_path, n_bit, 'ltp',
                                            'ltp_vts_testcases.txt')

        with open(case_definition_path, 'r') as f:
            for line in f:
                items = line.split('\t')
                if not len(items) == 4:
                    continue

                testsuite, testname, testbinary, arg = items
                args = arg.strip().split(',')

                if testname.startswith("DISABLED_"):
                    continue

                testcase = test_case.TestCase(
                    testsuite, testname, os.path.join(ltp_dir, 'testcases/bin',
                                                      testbinary), args)

                if self.IsTestcaseInList(testcase, ltp_configs.DISABLED_TESTS,
                                         n_bit):
                    continue

                if (not run_staging and self.IsTestcaseInList(
                        testcase, ltp_configs.STAGING_TESTS, n_bit)):
                    continue

                yield testcase
