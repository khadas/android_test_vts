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

from vts.runners.host import asserts
from vts.runners.host import const
from vts.testcases.kernel.ltp.shell_environment import shell_environment
from vts.testcases.kernel.ltp import ltp_enums
from vts.testcases.kernel.ltp import ltp_configs
from vts.testcases.kernel.ltp import requirements


class EnvironmentRequirementChecker(object):
    """LTP testcase environment checker.

    This class contains a dictionary for some known environment
    requirements for a set of test cases and several environment
    check functions to be mapped with. All check functions' results
    are cached in a dictionary for multiple use.

    Attributes:
        _REQUIREMENT_DEFINITIONS: dictionary {string, obj}, a map between
            requirement name and the actual definition class object
        _result_cache: dictionary {requirement_check_method_name:
            (bool, string)}, a map between check method name and cached result
            tuples (boolean, note)
        _shell_env: ShellEnvironment object, which checks and sets
            shell environments given a shell mirror
        shell: shell mirror object, can be used to execute shell
            commands on target side through runner
    """

    def __init__(self, shell):
        self.shell = shell
        self._result_cache = {}
        self._shell_env = shell_environment.ShellEnvironment(self.shell)
        self._REQUIREMENT_DEFINITIONS = requirements.GetRequrementDefinitions()

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
        result = copy.copy(ltp_configs.REQUIREMENT_FOR_ALL)

        for rule in ltp_configs.REQUIREMENTS_TO_TESTCASE:
            if str(test_case) in ltp_configs.REQUIREMENTS_TO_TESTCASE[rule]:
                result.append(rule)

        for rule in ltp_configs.REQUIREMENT_TO_TESTSUITE:
            if test_case.testsuite in ltp_configs.REQUIREMENT_TO_TESTSUITE[
                    rule]:
                result.append(rule)

        return list(set(result))

    def Check(self, test_case):
        """Check whether a given test case's requirement has been satisfied.
        Skip the test if not.

        Args:
            test_case: TestCase object, a given test case to check
        """
        asserts.skipIf(test_case.requirement_state ==
                       ltp_enums.RequirementState.UNSATISFIED, test_case.note)
        asserts.skipIf(not self.IsTestBinaryExist(test_case), test_case.note)

        for requirement in self.GetRequirements(test_case):
            if requirement not in self._result_cache:
                definitions = self._REQUIREMENT_DEFINITIONS[requirement]
                self._result_cache[
                    requirement] = self._shell_env.ExecuteDefinitions(
                        definitions)

            result, note = self._result_cache[requirement]
            logging.info("Result for %s's requirement %s is %s", test_case,
                         requirement, result)
            if result is False:
                test_case.requirement_state = ltp_enums.RequirementState.UNSATISFIED
                test_case.note = note
                asserts.skip(note)
        test_case.requirement_state = ltp_enums.RequirementState.SATISFIED

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
            test_case.requirement_state = ltp_enums.RequirementState.PATHEXISTS
        else:
            test_case.requirement_state = ltp_enums.RequirementState.UNSATISFIED
            test_case.note = "Test binary is not compiled."

    def IsTestBinaryExist(self, test_case):
        """Check whether the given test case's binary exists.

        Args:
            test_case: TestCase, the object representing the test case

        Return:
            True if exists, False otherwise
        """
        if test_case.requirement_state != ltp_enums.RequirementState.UNCHECKED:
            return test_case.requirement_state != ltp_enums.RequirementState.UNSATISFIED

        command_results = self._shell.Execute("ls %s" % test_case.path)
        exists = command_results[const.STDOUT][0].find(
            ": No such file or directory") > 0
        self.SetCheckResultPathExists(test_case, exists)
        return exists
