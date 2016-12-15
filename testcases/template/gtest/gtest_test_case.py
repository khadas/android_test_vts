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
import ntpath


class GtestTestCase(object):
    '''A class to represent a gtest test case.

    Attributes:
        test_suite: string, test suite name
        test_name: string, test case name which does not include test suite
        path: string, absolute test binary path on device
        tag: string, test tag
        output_file_name: string, gtest output xml file name
    '''

    def __init__(self, test_suite, test_name, path, tag=''):
        self.test_suite = test_suite
        self.test_name = test_name
        self.path = path
        self.tag = tag
        self.output_file_name = None

    def __str__(self):
        return '{}{}'.format(self.GetGtestName(), self.tag)

    def GetGtestName(self):
        '''Get a string that represents test name in gtest.

        Returns:
            A string test name in format '<test suite>.<test case>'
        '''
        return '{}{}'.format(self.test_suite, self.test_name)

    def GetGtestOutputFileName(self):
        '''Get gtest output xml's file name.'''
        if not self.output_file_name:
            self.output_file_name = '{path}-{name}.xml'.format(
                path=self.path.replace(os.path.sep, '_'), name=str(self))
        return self.output_file_name

    def GetRunCommand(self, output_file_name=None):
        if output_file_name:
            self.output_file_name = output_file_name
        return ('{path} --gtest_filter={test} '
                '--gtest_output=xml:{output_file_name}').format(
                    path=self.path,
                    test=self.GetGtestName(),
                    output_file_name=self.GetGtestOutputFileName())

    @property
    def output_file_name(self):
        """Get output_file_name"""
        return self._output_file_name

    @output_file_name.setter
    def output_file_name(self, output_file_name):
        """Set output_file_name"""
        self._output_file_name = output_file_name

    @property
    def test_suite(self):
        """Get test_suite"""
        return self._test_suite

    @test_suite.setter
    def test_suite(self, test_suite):
        """Set test_suite"""
        self._test_suite = test_suite

    @property
    def test_name(self):
        """Get test_name"""
        return self._test_name

    @test_name.setter
    def test_name(self, test_name):
        """Set test_name"""
        self._test_name = test_name

    @property
    def path(self):
        """Get path"""
        return self._path

    @path.setter
    def path(self, path):
        """Set path"""
        self._path = path

    @property
    def tag(self):
        """Get tag"""
        return self._tag

    @tag.setter
    def tag(self, tag):
        """Set tag"""
        self._tag = tag
