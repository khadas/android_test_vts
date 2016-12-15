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
import operator


class BinaryTestCase(object):
    '''A class to represent a binary test case.

    Attributes:
        test_suite: string, test suite name
        test_name: string, test case name which does not include test suite
        path: string, absolute test binary path on device
        tag: string, test tag
        put_tag_func: function that takes a name and tag to output a combination
    '''

    def __init__(self,
                 test_suite,
                 test_name,
                 path,
                 tag='',
                 put_tag_func=operator.add):
        self.test_suite = test_suite
        self.test_name = test_name
        self.path = path
        self.tag = tag
        self.put_tag_func = put_tag_func

    def __str__(self):
        return self.put_tag_func(self.GetFullName(), self.tag)

    def GetFullName(self):
        '''Get a string that represents the test.

        Returns:
            A string test name in format '<test suite>.<test name>' if
            test_suite is not empty; '<test name>' otherwise
        '''
        return '{}.{}'.format(
            self.test_suite,
            self.test_name) if self.test_suite else self.test_name

    def GetRunCommand(self):
        '''Get the command to run the test.

        Returns:
            String, a command to run the test.
        '''
        return self.path

    @property
    def test_suite(self):
        '''Get test_suite'''
        return self._test_suite

    @test_suite.setter
    def test_suite(self, test_suite):
        '''Set test_suite'''
        self._test_suite = test_suite

    @property
    def test_name(self):
        '''Get test_name'''
        return self._test_name

    @test_name.setter
    def test_name(self, test_name):
        '''Set test_name'''
        self._test_name = test_name

    @property
    def path(self):
        '''Get path'''
        return self._path

    @path.setter
    def path(self, path):
        '''Set path'''
        self._path = path

    @property
    def tag(self):
        '''Get tag'''
        return self._tag

    @tag.setter
    def tag(self, tag):
        '''Set tag'''
        self._tag = tag
