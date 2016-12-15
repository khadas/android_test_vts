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

import copy
import logging
import os

from vts.runners.host import asserts
from vts.runners.host import const
from vts.testcases.kernel.ltp.shell_environment import ShellEnvironment
from vts.testcases.kernel.ltp.shell_environment import CheckDefinition

LTPTMP = "/data/local/tmp/ltp/temp"

REQUIREMENTS_TO_TESTCASE = {"loop_device_support": ["syscalls-mount01",
                                                    "syscalls-fchmod06",
                                                    "syscalls-ftruncate04",
                                                    "syscalls-ftruncate04_64",
                                                    "syscalls-inotify03",
                                                    "syscalls-link08",
                                                    "syscalls-linkat02",
                                                    "syscalls-mkdir03",
                                                    "syscalls-mkdirat02",
                                                    "syscalls-mknod07",
                                                    "syscalls-mknodat02",
                                                    "syscalls-mmap16",
                                                    "syscalls-mount01",
                                                    "syscalls-mount02",
                                                    "syscalls-mount03",
                                                    "syscalls-mount04",
                                                    "syscalls-mount06",
                                                    "syscalls-rename11",
                                                    "syscalls-renameat01",
                                                    "syscalls-rmdir02",
                                                    "syscalls-umount01",
                                                    "syscalls-umount02",
                                                    "syscalls-umount03",
                                                    "syscalls-umount2_01",
                                                    "syscalls-umount2_02",
                                                    "syscalls-umount2_03",
                                                    "syscalls-utime06",
                                                    "syscalls-utimes01",
                                                    "syscalls-mkfs01", ]}

REQUIREMENT_FOR_ALL = ["ltptmp_dir"]

REQUIREMENT_TO_TESTSUITE = {}


class RequirementState(object):
    """Enum for test case requirement check state

    Attributes:
        UNCHECKED: test case requirement has not been checked
        PATHEXISTS: the path of the test case has been verified exist, but
                    all the other requirements have not been checked
        SATISFIED: all the requirements are satisfied
        UNSATISFIED: some of the requirements are not satisfied. Test case will
                     not be executed
    """
    UNCHECKED = 0
    PATHEXISTS = 1
    SATISFIED = 2
    UNSATISFIED = 3


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
        self.requirement_state = RequirementState.UNCHECKED
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

    def __str__(self):
        return "%s-%s" % (self.testsuite, self.testname)


class TestCasesParser(object):
    """Load a ltp vts testcase definition file and parse it into a generator.

    Attributes:
        _data_path: string, the vts data path on host side
    """

    def __init__(self, data_path):
        self._data_path = data_path

    def _GetTestcaseFilePath(self):
        """return the test case definition fiile's path."""
        return os.path.join(self._data_path, '32', 'ltp',
                            'ltp_vts_testcases.txt')

    def Load(self, ltp_dir):
        """read the definition file and return a TestCase generator."""
        with open(self._GetTestcaseFilePath(), 'r') as f:
            for line in f:
                items = line.split('\t')
                if not len(items) == 4:
                    continue
                testsuite, testname, testbinary, arg = items
                arg = arg.strip()

                if testname.startswith("DISABLED_"):
                    continue

                args = []
                if arg:
                    args.extend(arg.split(','))
                yield TestCase(
                    testsuite, testname,
                    os.path.join(ltp_dir, 'testcases/bin', testbinary), args)


class EnvironmentRequirementChecker(object):
    """LTP testcase environment checker.

    This class contains a dictionary for some known environment
    requirements for a set of test cases and several environment
    check functions to be mapped with. All check functions' results
    are cached in a dictionary for multiple use.

    Attributes:
        _REQUIREMENT_DEFINITIONS: dictionary {string, method}, a map between requirement
                                  name and the actual check method inside class
        _result_cache: dictionary {requirement_check_method_name: (bool, string)}
                       a map between check method name and cached result
                       tuples (boolean, note)
        _shell_env: ShellEnvironment object, which checks and sets shell environments
                    given a shell mirror
        shell: shell mirror object, can be used to execute shell commands on target
               side through runner
    """

    def __init__(self, shell):
        self.shell = shell
        self._result_cache = {}

        self._shell_env = ShellEnvironment(self.shell)
        loop_device_support = CheckDefinition(self._shell_env.LoopDeviceSupport)
        ltptmp_dir = CheckDefinition(self._shell_env.DirsAllExistAndPermission,
                                     True, True, [LTPTMP, "%s/tmp" % LTPTMP],
                                     [775, 775])
        self._REQUIREMENT_DEFINITIONS = {
            "loop_device_support": loop_device_support,
            "ltptmp_dir": ltptmp_dir
        }

    @property
    def shell(self):
        """Get the runner's shell mirror object to execute commands"""
        return self._shell

    @shell.setter
    def shell(self, shell):
        """Set the runner's shell mirror object to execute commands"""
        self._shell = shell

    def Cleanup(self):
        """Run all cleanup jobs at the end of tests"""
        self._shell_env.Cleanup()

    def GetRequirements(self, test_case):
        """Get a list of requirements for a fiven test case

        Args:
            test_case: TestCase object, the test case to query
        """
        result = copy.copy(REQUIREMENT_FOR_ALL)

        for rule in REQUIREMENTS_TO_TESTCASE:
            if str(test_case) in REQUIREMENTS_TO_TESTCASE[rule]:
                result.append(rule)

        for rule in REQUIREMENT_TO_TESTSUITE:
            if test_case.testsuite in REQUIREMENT_TO_TESTSUITE[rule]:
                result.append(rule)

        return list(set(result))

    def Check(self, test_case):
        """Check whether a given test case's requirement has been satisfied.
        Skip the test if not.

        Args:
            test_case: TestCase object, a given test case to check
        """
        asserts.skipIf(
            test_case.requirement_state == RequirementState.UNSATISFIED,
            test_case.note)
        asserts.skipIf(not self.IsTestBinaryExist(test_case),
                       test_case.note)

        for requirement_name in self.GetRequirements(test_case):
            if requirement_name not in self._result_cache:
                req_def = self._REQUIREMENT_DEFINITIONS[requirement_name]
                self._result_cache[requirement_name] = req_def.Execute()
            result, note = self._result_cache[requirement_name]
            logging.info("Result for %s's requirement %s is %s", test_case,
                         requirement_name, result)
            if result is False:
                test_case.requirement_state = RequirementState.UNSATISFIED
                test_case.note = note
                asserts.skip(note)
        test_case.requirement_state = RequirementState.SATISFIED

    def RunCheckTestcasePathExistsAll(self, test_cases):
        """Run a batch job to check the path existance of all given test cases

        Args:
            test_case: list of TestCase objects.
        """
        commands = ["ls %s" % test_case.path for test_case in test_cases]
        command_results = self._shell.Execute(commands)
        exists_results = [exit_code is 0
                          for exit_code in command_results[const.EXIT_CODE]]
        for testcase_result in zip(test_cases, exists_results):
            self.SetCheckResultPathExists(*testcase_result)

    def RunChmodTestcasesAll(self, test_cases):
        """Batch commands to make all test case binaries executable
        Test cases whose binary path does not exist will be excluded.

        Args:
            test_case: list of TestCase objects.
        """
        self._shell.Execute(["chmod 775 %s" % test_case.path
                             for test_case in test_cases
                             if self.IsTestBinaryExist(test_case)])

    def SetCheckResultPathExists(self, test_case, is_exists):
        """Set a path exists result to a test case object

        Args:
            test_case: TestCase object, the target test case
            is_exists: bool, the result. True for exists, False otherwise.
        """
        if is_exists:
            test_case.requirement_state = RequirementState.PATHEXISTS
        else:
            test_case.requirement_state = RequirementState.UNSATISFIED
            test_case.note = "Test binary is not compiled."

    def IsTestBinaryExist(self, test_case):
        """Check whether the given test case's binary exists.

        Args:
            test_case: TestCase, the object representing the test case
        Return:
            True if exists, False otherwise
        """
        if test_case.requirement_state != RequirementState.UNCHECKED:
            return test_case.requirement_state != RequirementState.UNSATISFIED

        command_results = self._shell.Execute("ls %s" % test_case.path)
        exists = command_results[const.STDOUT][0].find(
            ": No such file or directory") > 0
        self.SetCheckResultPathExists(test_case, exists)
        return exists
