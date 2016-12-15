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
import uuid
import re

from vts.runners.host import utils


def PutTag(name, tag):
    '''Put tag on name and return the resulting string.

    Args:
        name: string, a test name
        tag: string

    Returns:
        String, the result string after putting tag on the name
    '''
    return '{}{}'.format(name, tag)


class GtestTestCase(object):
    '''A class to represent a gtest test case.

    Attributes:
        test_suite: string, test suite name
        test_name: string, test case name which does not include test suite
        path: string, absolute test binary path on device
        tag: string, test tag
        output_file_path: string, gtest output xml file name
    '''

    def __init__(self, test_suite, test_name, path, tag=''):
        self.test_suite = test_suite
        self.test_name = test_name
        self.path = path
        self.tag = tag
        self.output_file_path = 'gtest_output_{name}.xml'.format(
            name=re.sub(r'\W+', '_', str(self)))

    def __str__(self):
        return PutTag(self.GetGtestName(), self.tag)

    def GetGtestName(self):
        '''Get a string that represents test name in gtest.

        Returns:
            A string test name in format '<test suite>.<test case>'
        '''
        return '{}{}'.format(self.test_suite, self.test_name)

    def GetRunCommand(self, output_file_path=None):
        if output_file_path:
            self.output_file_path = output_file_path
        return ('{path} --gtest_filter={test} '
                '--gtest_output=xml:{output_file_path}').format(
                    path=self.path,
                    test=self.GetGtestName(),
                    output_file_path=self.output_file_path)

    @property
    def output_file_path(self):
        """Get output_file_path"""
        return self._output_file_path

    @output_file_path.setter
    def output_file_path(self, output_file_path):
        """Set output_file_path.

        Lengths of both file name and path will be checked. If longer than
        maximum allowance, file name will be set to a random name, and
        directory will be set to relative directory.

        Args:
            output_file_path: string, intended path of output xml file
        """
        output_file_path = os.path.normpath(output_file_path)

        if len(ntpath.basename(output_file_path)) > utils.MAX_FILENAME_LEN:
            logging.error(
                'File name of output file "{}" is longer than {}.'.format(
                    output_file_path), utils.MAX_FILENAME_LEN)
            output_file_path = os.path.join(
                ntpath.dirname(output_file_path),
                '{}.xml'.format(uuid.uuid4()))
            logging.info('Output file path is set as "%s".', output_file_path)

        if len(output_file_path) > utils.MAX_PATH_LEN:
            logging.error(
                'File path of output file "{}" is longer than {}.'.format(
                    output_file_path), utils.MAX_PATH_LEN)
            output_file_path = ntpath.basename(output_file_path)
            logging.info('Output file path is set as "%s".',
                         os.path.abspath(output_file_path))

        self._output_file_path = output_file_path

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
