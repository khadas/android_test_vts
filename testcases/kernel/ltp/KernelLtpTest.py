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

import random
import logging
from concurrent import futures
import queue
import threading

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device

from vts.testcases.kernel.ltp import test_cases_parser
from vts.testcases.kernel.ltp import environment_requirement_checker as env_checker
from vts.testcases.kernel.ltp.shell_environment import shell_environment
from vts.testcases.kernel.ltp import ltp_enums
from vts.testcases.kernel.ltp import ltp_configs


class KernelLtpTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Runs the LTP (Linux Test Project) test cases against Android OS kernel.

    Attributes:
        _dut: AndroidDevice, the device under test
        _shell: ShellMirrorObject, shell mirror object used to execute commands
        _testcases: TestcasesParser, test case input parser
        _env: dict<stirng, string>, dict of environment variable key value pair
        data_file_path: string, runner's directory where test cases are stored
        run_staging: bool, whether to run staging tests
        number_of_threads: int, number of threads to run in parallel. If this
                           number is set to 0, the test case will automatically
                           pick the number of available CPUs on device. If
                           the number is less than 0, it will be set to 1. If
                           the number is greater than 0, that number of threads
                           will be created to run the tests.
        include_filter: list of string, a list of test case names to run
        exclude_filter: list of string, a list of test case names to exclude
    """
    _32BIT = "32"
    _64BIT = "64"
    _PASS = 0
    _SKIP = 1
    _FAIL = -1

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.KEY_TEST_SUITE, ltp_enums.ConfigKeys.RUN_STAGING,
            ltp_enums.ConfigKeys.RUN_32BIT, ltp_enums.ConfigKeys.RUN_64BIT,
            ltp_enums.ConfigKeys.NUMBER_OF_THREADS
        ]
        self.getUserParams(required_params)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)
        logging.info("%s: %s", keys.ConfigKeys.KEY_TEST_SUITE, self.test_suite)
        logging.info("%s: %s", ltp_enums.ConfigKeys.RUN_STAGING,
                     self.run_staging)
        logging.info("%s: %s", ltp_enums.ConfigKeys.NUMBER_OF_THREADS,
                     self.number_of_threads)

        self.include_filter = self.test_suite[
            keys.ConfigKeys.KEY_INCLUDE_FILTER]
        logging.info("%s: %s", keys.ConfigKeys.KEY_INCLUDE_FILTER,
                     self.include_filter)

        self.exclude_filter = self.test_suite[
            keys.ConfigKeys.KEY_EXCLUDE_FILTER]
        logging.info("%s: %s", keys.ConfigKeys.KEY_EXCLUDE_FILTER,
                     self.exclude_filter)

        self._dut = self.registerController(android_device)[0]
        logging.info("product_type: %s", self._dut.product_type)
        self._dut.shell.InvokeTerminal("one")
        self.shell = self._dut.shell.one

        self._requirement = env_checker.EnvironmentRequirementChecker(
            self.shell)
        self._shell_env = shell_environment.ShellEnvironment(self.shell)

        self._testcases = test_cases_parser.TestCasesParser(
            self.data_file_path, self.include_filter, self.exclude_filter)
        self._env = {ltp_enums.ShellEnvKeys.TMP: ltp_configs.TMP,
                     ltp_enums.ShellEnvKeys.TMPBASE: ltp_configs.TMPBASE,
                     ltp_enums.ShellEnvKeys.LTPTMP: ltp_configs.LTPTMP,
                     ltp_enums.ShellEnvKeys.TMPDIR: ltp_configs.TMPDIR,
                     ltp_enums.ShellEnvKeys.LTP_DEV_FS_TYPE:
                     ltp_configs.LTP_DEV_FS_TYPE,
                     ltp_enums.ShellEnvKeys.LTPROOT: ltp_configs.LTPDIR,
                     ltp_enums.ShellEnvKeys.PATH: ltp_configs.PATH}

    @property
    def shell(self):
        """returns an object that can execute a shell command"""
        return self._shell

    @shell.setter
    def shell(self, shell):
        """Set shell object"""
        self._shell = shell

    def PreTestSetup(self):
        """Setups that needs to be done before any tests."""
        find_sh_command = "find %s -name '*.sh' -print0" % ltp_configs.LTPBINPATH

        # Required for executing any shell scripts in Android
        binsh_sed_pattern = self._shell_env.CreateSedPattern(
            '#!/bin/sh', '#!/system/bin/sh')
        # Required for using dd command in Android
        dd_sed_pattern = self._shell_env.CreateSedPattern('bs=1M', 'bs=1m')
        # Required for creating pid files since /var is not available in Android
        pid_sed_pattern = self._shell_env.CreateSedPattern('/var/run',
                                                           ltp_configs.TMP)

        patterns = (binsh_sed_pattern, dd_sed_pattern, pid_sed_pattern)

        replace_commands = ["{find} | xargs -0 sed -i \'{pattern}\'".format(
            find=find_sh_command, pattern=pattern) for pattern in patterns]

        results = self.shell.Execute(replace_commands)
        asserts.assertFalse(
            any(results[const.EXIT_CODE]),
            "Error: pre-test setup failed. "
            "Commands: {commands}. Results: {results}".format(
                commands=replace_commands, results=results))

        self._report_thread_lock = threading.Lock()

    def PushFiles(self, n_bit):
        """Push the related files to target.

        Args:
            n_bit: int, _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """

        self.shell.Execute("mkdir %s -p" % ltp_configs.LTPDIR)
        self._dut.adb.push("%s/%s/ltp/. %s" %
                           (self.data_file_path, n_bit, ltp_configs.LTPDIR))

    def GetEnvp(self):
        """Generate the environment variable required to run the tests."""
        return ' '.join("%s=%s" % (key, value)
                        for key, value in self._env.items())

    def tearDownClass(self):
        """Deletes all copied data files."""
        self.shell.Execute("rm -rf %s" % ltp_configs.LTPDIR)
        self._requirement.Cleanup()

    def Verify(self, results):
        """Interpret the test result of each test case.

        Returns:
            tuple(int, string), a tuple of int which represents test pass, fail
            or skip, and string representing the reason of a failed or skipped
            test
        """
        if not results:
            return (self._FAIL, "No response received. Socket timeout")

        # For LTP test cases, we run one shell command for each test case
        # So the result should also contains only one execution output
        stdout = results[const.STDOUT][0]
        ret_code = results[const.EXIT_CODE][0]
        # Test case is not for the current configuration, SKIP
        if ((ret_code == ltp_enums.TestExitCode.TCONF and
             'TPASS' not in stdout) or
            (ret_code == ltp_enums.TestExitCode.TPASS and 'CONF' in stdout)):
            return (self._SKIP, "Incompatible test skipped: TCONF")
        elif ret_code not in (ltp_enums.TestExitCode.TCONF,
                              ltp_enums.TestExitCode.TPASS):
            return (self._FAIL,
                    "Got return code %s, test did not pass." % ret_code)
        else:
            return (self._PASS, None)

    def CheckResult(self, cmd_results, result=None, note=None):
        """Check a test result and emit exceptions if test failed or skipped.

        If the shell command result is not yet interpreted, self.Verify will
        be called to interpret the results.

        Args:
            cmd_results: dict([str],[str],[int]), command results from shell.
            result: int, which is one of the values of _PASS, _SKIP, and _FAIL
            note: string, reason why a test failed or get skipped
        """
        logging.info("Checking results: '{}', '{}', '{}'".format(cmd_results,
                                                                 result, note))
        asserts.assertTrue(cmd_results, "No response received. Socket timeout")

        logging.info("stdout: %s", cmd_results[const.STDOUT])
        logging.info("stderr: %s", cmd_results[const.STDERR])
        logging.info("exit_code: %s", cmd_results[const.EXIT_CODE])

        if result is None:
            result, note = self.Verify(cmd_results)

        asserts.skipIf(result == self._SKIP, note)
        asserts.assertEqual(result, self._PASS, note)

    def TestNBits(self, n_bit):
        """Runs all 32-bit or 64-bit LTP test cases.

        Args:
            n_bit: _32BIT, or 32, for 32bit test;
                   _64BIT, or 64, for 64bit test;
        """
        self.PushFiles(n_bit)
        self.PreTestSetup()
        logging.info("[Test Case] test%sBits SKIP", n_bit)

        test_cases = list(
            self._testcases.Load(
                ltp_configs.LTPDIR, n_bit=n_bit, run_staging=self.run_staging))

        logging.info("Checking binary exists for all test cases.")
        self._requirement.CheckAllTestCaseExecutables(test_cases)
        logging.info("Start running %i individual tests." % len(test_cases))

        self.RunGeneratedTestsMultiThread(
            test_func=self.RunLtpOnce,
            settings=test_cases,
            args=(n_bit, ),
            name_func=self.GetTestName)
        logging.info("[Test Case] test%sBits", n_bit)
        asserts.skip("Finished generating {} bit tests.".format(n_bit))

    def RunGeneratedTestsMultiThread(self, test_func, settings, args,
                                     name_func):
        """Run LTP tests with multi-threads.

        If number_of_thread is specified to be 0 in config file, a shell query
        will be made to the device to get the number of available CPUs. If
        number_of_thread or number of CPUs available is 1, this function will
        call and return parent class's regular runGeneratedTest function. Since
        some tests may be competing resources with each other, all the failed
        tests will be rerun sequentially in the end to confirm their failure.
        Also, if include_filter is not empty, only 1 thread will be running.

        Args:
            test_func: The common logic shared by all these generated test
                       cases. This function should take at least one argument,
                       which is a parameter set.
            settings: A list of strings representing parameter sets. These are
                      usually json strings that get loaded in the test_func.
            args: Iterable of additional position args to be passed to
                  test_func.
            name_func: A function that takes a test setting and generates a
                       proper test name.

        Returns:
            A list of settings that fail.
        """
        n_workers = self.number_of_threads

        if n_workers < 0:
            logging.error('invalid setting for number of threads: < 0.')
            n_workers = 1

        # Include filter is not empty; Run in sequential.
        if self.include_filter:
            n_workers = 1

        # Number of thread is set to 0 (automatic)
        if not n_workers:
            n_workers = self._shell_env.GetDeviceNumberOfPresentCpu()
            logging.info('Number of CPU available on device: %i', n_workers)

        # Skip multithread version if only 1 worker available
        if n_workers == 1:
            return self.runGeneratedTests(
                test_func=test_func,
                settings=settings,
                args=args,
                name_func=name_func)

        # Shuffle the tests to reduce resource competition probability
        random.shuffle(settings)

        # Create a queue for thread workers to pull tasks
        q = queue.Queue()
        map(q.put, settings)

        # Create individual shell sessions for thread workers
        for i in xrange(n_workers):
            self._dut.shell.InvokeTerminal("shell_thread_{}".format(i))

        failed_tests = set()
        with futures.ThreadPoolExecutor(max_workers=n_workers) as executor:
            fs = [executor.submit(self.RunLtpWorker, q, args, name_func, i)
                  for i in xrange(n_workers)]

            failed_test_sets = map(futures.Future.result, fs)
            for failed_test_set in failed_test_sets:
                for test_case in failed_test_set:
                    failed_tests.add(test_case)

        for test_case in failed_tests:
            logging.info(
                "Test case %s failed during multi-thread run, rerunning...",
                test_case)

        # In the end, rerun all failed tests to confirm their failure
        # in sequential.
        return self.runGeneratedTests(
            test_func=test_func,
            settings=failed_tests,
            args=args,
            name_func=name_func)

    def RunLtpWorker(self, testcases, args, name_func, id):
        """Worker thread to run a LTP test case at a time."""
        shell = getattr(self._dut.shell, "shell_thread_{}".format(id))
        failed_tests = set()

        while True:
            test_case = None
            try:
                test_case = testcases.get(block=False)
                logging.info("Worker {} takes '{}'.".format(id, test_case))
            except:
                logging.info("Worker {} finished.".format(id))
                return failed_tests

            test_name = name_func(test_case, *args)

            logging.info("Worker {} starts checking requirement "
                         "for '{}'.".format(id, test_case))
            requirement_satisfied = self._requirement.Check(test_case)
            if not requirement_satisfied:
                logging.info("Worker {} reports requirement "
                             "not satisfied for '{}'.".format(id, test_case))
                self.InternalResultReportMultiThread(test_name, asserts.skipIf,
                                                     (False, test_case.note))
                continue

            cmd = "export {envp} && {commands}".format(
                envp=self.GetEnvp(), commands=test_case.GetCommand())

            logging.info("Worker {} starts executing command "
                         "for '{}'.\n  Command:{}".format(id, test_case, cmd))
            cmd_results = shell.Execute(cmd)

            logging.info("Worker {} starts verifying results "
                         "for '{}'.".format(id, test_case))

            result, note = self.Verify(cmd_results)
            if result == self._FAIL:
                # Hide failed tests from the runner and put into rerun list
                logging.info("Worker {} reports '{}' failed. Adding to "
                             "sequential job queue.".format(id, test_case))
                failed_tests.add(test_case)
            else:
                # Report skipped or passed tests to runner
                self.InternalResultReportMultiThread(
                    test_name, self.CheckResult, (cmd_results, result, note))

    def InternalResultReportMultiThread(self, test_name, function, args,
                                        **kwargs):
        """Report a test result to runner thread safely.

        Run the given function to generate result for the runner. The function
        given should produce the same result visible to the runner but may not
        run any actual tests.

        Args:
            test_name: string, name of a test case
            function: the function to generate a test case result for runner
            args: any arguments for the function
            **kwargs: any additional keyword arguments for runner
        """
        self._report_thread_lock.acquire()
        self.results.requested.append(test_name)
        try:
            self.execOneTest(test_name, function, args, **kwargs)
        except Exception as e:
            raise e
        finally:
            self._report_thread_lock.release()

    def GetTestName(self, test_case, n_bit):
        "Generate the vts test name of a ltp test"
        return "{}_{}bit".format(test_case, n_bit)

    def RunLtpOnce(self, test_case, n_bit):
        "Run one LTP test case"
        asserts.skipIf(not self._requirement.Check(test_case), test_case.note)

        cmd = "export {envp} && {commands}".format(
            envp=self.GetEnvp(), commands=test_case.GetCommand())
        logging.info("Executing %s", cmd)
        self.CheckResult(self.shell.Execute(cmd))

    def test32Bits(self):
        """Runs all 32-bit LTP test cases."""
        asserts.skipIf(not self.run_32bit,
                       'User specified not to run 32 bit version LTP tests.')
        self.TestNBits(self._32BIT)

    def test64Bits(self):
        """Runs all 64-bit LTP test cases."""
        asserts.skipIf(not self.run_64bit,
                       'User specified not to run 64 bit version LTP tests.')
        asserts.skipIf(not self._shell_env.IsDeviceArch64Bit(),
                       'Target device does not support 64 bit tests.')

        self.TestNBits(self._64BIT)


if __name__ == "__main__":
    test_runner.main()
