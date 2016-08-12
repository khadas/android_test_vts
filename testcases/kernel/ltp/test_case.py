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

from vts.testcases.kernel.ltp import ltp_enums


class TestCase(object):
    """Stores name, path, and param information for each test case.

    Attributes:
        testsuite: string, name of testsuite to which the testcase belongs
        testname: string, name of the test case
        path: string, test binary path
        _args: list of string, test case command line arguments
        requirement_state: RequirementState, enum representing requirement
                            check results
        note: string, a place to store additional note for the test case
              such as what environment requirement did not satisfy.
    """

    def __init__(self, testsuite, testname, path, args):
        self.testsuite = testsuite
        self.testname = testname
        self.path = path
        self._args = args
        self.requirement_state = ltp_enums.RequirementState.UNCHECKED
        self.note = None

    @property
    def note(self):
        """Get the note"""
        return self._note

    @note.setter
    def note(self, note):
        """Set the note"""
        self._note = note

    @property
    def requirement_state(self):
        """Get the requirement state"""
        return self._requirement_state

    @requirement_state.setter
    def requirement_state(self, requirement_state):
        """Set the requirement state"""
        self._requirement_state = requirement_state

    @property
    def testsuite(self):
        """Get the test suite's name."""
        return self._testsuite

    @testsuite.setter
    def testsuite(self, testsuite):
        """Set the test suite's name."""
        self._testsuite = testsuite

    @property
    def testname(self):
        """Get the test case's name."""
        return self._testname

    @testname.setter
    def testname(self, testname):
        """Set the test case's name."""
        self._testname = testname

    @property
    def path(self):
        """Get the test binary's file name."""
        return self._path

    @path.setter
    def path(self, path):
        """Set the test testbinary's name."""
        self._path = path

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

    @property
    def fullname(self):
        """Return full test name in <testsuite-testname> format"""
        return "%s-%s" % (self.testsuite, self.testname)

    def __str__(self):
        return self.fullname
