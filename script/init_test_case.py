#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Initiate a test case directory.
# This script copy a template which contains Android.mk, __init__.py files,
# AndroidTest.xml and a test case python file into a given relative directory
# under testcases/ using the given test name.

import os
import sys
import datetime
import re
import shutil
import argparse

import init_test_case_template as template

VTS_PATH = 'test/vts'
VTS_TEST_CASE_PATH = os.path.join(VTS_PATH, 'testcases')
PYTHON_INIT_FILE_NAME = '__init__.py'
ANDROID_MK_FILE_NAME = 'Android.mk'
ANDROID_TEST_XML_FILE_NAME = 'AndroidTest.xml'


class TestCaseCreator(object):
    '''Init a test case directory with helloworld test case.

    Attributes:
        test_name: string, test case name in UpperCamel
        test_dir_under_testcases: string, test case relative directory unter
                                  test/vts/testcases

        test_name: string, test case name in UpperCamel
        build_top: string, equal to environment variable ANDROID_BUILD_TOP
        test_dir: string, test case absolute directory
        test_name: string, test case name in UpperCamel
        test_type: test type, such as HidlHalTest, HostDrivenTest, etc
        current_year: current year
        vts_test_case_dir: absolute dir of vts testcases directory
    '''

    def __init__(self, test_name, test_dir_under_testcases, test_type):
        if not test_dir_under_testcases:
            print 'Error: Empty test directory entered. Exiting'
            sys.exit(3)
        self.test_dir_under_testcases = os.path.normpath(
            test_dir_under_testcases.strip())

        if not self.IsUpperCamel(test_name):
            print 'Error: Test name not in UpperCamel case. Exiting'
            sys.exit(4)
        self.test_name = test_name

        if not test_type:
            self.test_type = 'HidlHalTest'
        else:
            self.test_type = test_type

        self.build_top = os.getenv('ANDROID_BUILD_TOP')
        if not self.build_top:
            print('Error: Missing ANDROID_BUILD_TOP env variable. Please run '
                  '\'. build/envsetup.sh; lunch <build target>\' Exiting...')
            sys.exit(1)

        self.vts_test_case_dir = os.path.abspath(
            os.path.join(self.build_top, VTS_TEST_CASE_PATH))

        self.test_dir = os.path.abspath(
            os.path.join(self.vts_test_case_dir,
                         self.test_dir_under_testcases))

        self.current_year = datetime.datetime.now().year

    def InitTestCaseDir(self):
        '''Start init test case directory'''
        if os.path.exists(self.test_dir):
            print 'Error: Test directory already exists. Exiting...'
            sys.exit(2)
        try:
            os.makedirs(self.test_dir)
        except:
            print('Error: Failed to create test directory at %s. '
                  'Exiting...' % self.test_dir)
            sys.exit(2)

        self.CreatePythonInitFile()
        self.CreateAndroidMk()
        self.CreateAndroidTestXml()
        self.CreateTestCasePy()

    def UpperCamelToLowerUnderScore(self, name):
        '''Convert UpperCamel name to lower_under_score name.

        Args:
            name: string in UpperCamel.

        Returns:
            a lower_under_score version of the given name
        '''
        return re.sub('(?!^)([A-Z]+)', r'_\1', name).lower()

    def IsUpperCamel(self, name):
        '''Check whether a given name is UpperCamel case.

        Args:
            name: string.

        Returns:
            True if name is in UpperCamel case, False otherwise
        '''
        regex = re.compile('((?:[A-Z][a-z]+)[0-9]*)+')
        match = regex.match(name)
        return match and (match.end() - match.start() == len(name))

    def CreatePythonInitFile(self):
        '''Populate test case directory and parent directories with __init__.py.
        '''
        if not self.test_dir.startswith(self.vts_test_case_dir):
            print 'Error: Test case directory is not under VTS test case directory.'
            sys.exit(4)

        path = self.test_dir
        while not path == self.vts_test_case_dir:
            target = os.path.join(path, PYTHON_INIT_FILE_NAME)
            if not os.path.exists(target):
                print 'Creating %s' % target
                with open(target, 'w') as f:
                    pass
            path = os.path.dirname(path)

    def CreateAndroidMk(self):
        '''Populate test case directory and parent directories with Android.mk
        '''
        vts_dir = os.path.join(self.build_top, VTS_PATH)

        target = os.path.join(self.test_dir, ANDROID_MK_FILE_NAME)
        with open(target, 'w') as f:
            print 'Creating %s' % target
            f.write(
                template.LICENSE_STATEMENT_POUND.format(
                    year=self.current_year))
            f.write('\n')
            f.write(
                template.ANDROID_MK_TEMPLATE.format(
                    test_name=self.test_name,
                    config_src_dir=self.test_dir[len(vts_dir) + 1:]))

        path = self.test_dir
        while not path == vts_dir:
            target = os.path.join(path, ANDROID_MK_FILE_NAME)
            if not os.path.exists(target):
                print 'Creating %s' % target
                with open(target, 'w') as f:
                    f.write(
                        template.LICENSE_STATEMENT_POUND.format(
                            year=self.current_year))
                    f.write(template.ANDROID_MK_CALL_SUB)
            path = os.path.dirname(path)

    def CreateAndroidTestXml(self):
        '''Create AndroidTest.xml'''
        target = os.path.join(self.test_dir, ANDROID_TEST_XML_FILE_NAME)
        with open(target, 'w') as f:
            print 'Creating %s' % target
            f.write(template.XML_HEADER)
            f.write(
                template.LICENSE_STATEMENT_XML.format(year=self.current_year))
            f.write(
                template.ANDROID_TEST_XML_TEMPLATE.format(
                    test_name=self.test_name,
                    test_type=self.test_type,
                    test_path_under_vts=self.test_dir[len(
                        os.path.join(self.build_top, VTS_PATH)) + 1:],
                    test_case_file_without_extension=self.UpperCamelToLowerUnderScore(
                        self.test_name)))

    def CreateTestCasePy(self):
        '''Create <test_case_name>.py'''
        target = os.path.join(self.test_dir, '%s.py' %
                              self.UpperCamelToLowerUnderScore(self.test_name))
        with open(target, 'w') as f:
            print 'Creating %s' % target
            f.write(template.PY_HEADER)
            f.write(
                template.LICENSE_STATEMENT_POUND.format(
                    year=self.current_year))
            f.write('\n')
            f.write(
                template.TEST_CASE_PY_TEMPLATE.format(
                    test_name=self.test_name))


def main():
    parser = argparse.ArgumentParser(description='Initiate a test case.')
    parser.add_argument(
        '--name',
        dest='test_name',
        required=True,
        help='Test case name in UpperCamel. Example: KernelLtpTest')
    parser.add_argument(
        '--dir',
        dest='test_dir',
        required=True,
        help='Test case relative directory under test/vts/testcses.')
    parser.add_argument(
        '--type',
        dest='test_type',
        required=False,
        help='Test type, such as HidlHalTest, HostDrivenTest, etc.')

    args = parser.parse_args()
    test_case_creater = TestCaseCreator(args.test_name, args.test_dir,
                                        args.test_type)
    test_case_creater.InitTestCaseDir()


if __name__ == '__main__':
    main()
