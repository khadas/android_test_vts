#!/usr/bin/env python3.4
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

import logging
import os

from vts.runners.host import asserts
from vts.runners.host import errors
from vts.runners.host import keys
from vts.runners.host import logger
from vts.runners.host import records
from vts.runners.host import signals
from vts.runners.host import utils

# Macro strings for test result reporting
TEST_CASE_TOKEN = "[Test Case]"
RESULT_LINE_TEMPLATE = TEST_CASE_TOKEN + " %s %s"
STR_TEST = "test"
STR_GENERATE = "generate"


class BaseTestClass(object):
    """Base class for all test classes to inherit from.

    This class gets all the controller objects from test_runner and executes
    the test cases requested within itself.

    Most attributes of this class are set at runtime based on the configuration
    provided.

    Attributes:
        tests: A list of strings, each representing a test case name.
        TAG: A string used to refer to a test class. Default is the test class
             name.
        results: A records.TestResult object for aggregating test results from
                 the execution of test cases.
        currentTestName: A string that's the name of the test case currently
                           being executed. If no test is executing, this should
                           be None.
    """

    TAG = None

    def __init__(self, configs):
        self.tests = []
        if not self.TAG:
            self.TAG = self.__class__.__name__
        # Set all the controller objects and params.
        for name, value in configs.items():
            setattr(self, name, value)
        self.results = records.TestResult()
        self.currentTestName = None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self._exec_func(self.cleanUp)

    def getUserParams(self, req_param_names=[], opt_param_names=[], **kwargs):
        """Unpacks user defined parameters in test config into individual
        variables.

        Instead of accessing the user param with self.user_params["xxx"], the
        variable can be directly accessed with self.xxx.

        A missing required param will raise an exception. If an optional param
        is missing, an INFO line will be logged.

        Args:
            req_param_names: A list of names of the required user params.
            opt_param_names: A list of names of the optional user params.
            **kwargs: Arguments that provide default values.
                e.g. getUserParams(required_list, opt_list, arg_a="hello")
                     self.arg_a will be "hello" unless it is specified again in
                     required_list or opt_list.

        Raises:
            BaseTestError is raised if a required user params is missing from
            test config.
        """
        for k, v in kwargs.items():
            setattr(self, k, v)
        for name in req_param_names:
            if name not in self.user_params:
                raise errors.BaseTestError(("Missing required user param '%s' "
                                            "in test configuration.") % name)
            setattr(self, name, self.user_params[name])
        for name in opt_param_names:
            if name not in self.user_params:
                logging.info(("Missing optional user param '%s' in "
                              "configuration, continue."), name)
            else:
                setattr(self, name, self.user_params[name])

    def _setUpClass(self):
        """Proxy function to guarantee the base implementation of setUpClass
        is called.
        """
        return self.setUpClass()

    def setUpClass(self):
        """Setup function that will be called before executing any test case in
        the test class.

        To signal setup failure, return False or raise an exception. If
        exceptions were raised, the stack trace would appear in log, but the
        exceptions would not propagate to upper levels.

        Implementation is optional.
        """
        pass

    def _tearDownClass(self):
        """Proxy function to guarantee the base implementation of tearDownClass
        is called.
        """
        return self.tearDownClass()

    def tearDownClass(self):
        """Teardown function that will be called after all the selected test
        cases in the test class have been executed.

        Implementation is optional.
        """
        pass

    def _setUpTest(self, test_name):
        """Proxy function to guarantee the base implementation of setUpTest is
        called.
        """
        self.currentTestName = test_name
        return self.setUpTest()

    def setUpTest(self):
        """Setup function that will be called every time before executing each
        test case in the test class.

        To signal setup failure, return False or raise an exception. If
        exceptions were raised, the stack trace would appear in log, but the
        exceptions would not propagate to upper levels.

        Implementation is optional.
        """

    def _tearDownTest(self, test_name):
        """Proxy function to guarantee the base implementation of tearDownTest
        is called.
        """
        try:
            self.tearDownTest()
        finally:
            self.currentTestName = None

    def tearDownTest(self):
        """Teardown function that will be called every time a test case has
        been executed.

        Implementation is optional.
        """

    def _onFail(self, record):
        """Proxy function to guarantee the base implementation of onFail is
        called.

        Args:
            record: The records.TestResultRecord object for the failed test
                    case.
        """
        test_name = record.test_name
        logging.error(record.details)
        begin_time = logger.epochToLogLineTimestamp(record.begin_time)
        logging.info(RESULT_LINE_TEMPLATE, test_name, record.result)
        self.onFail(test_name, begin_time)

    def onFail(self, test_name, begin_time):
        """A function that is executed upon a test case failure.

        User implementation is optional.

        Args:
            test_name: Name of the test that triggered this function.
            begin_time: Logline format timestamp taken when the test started.
        """

    def _onPass(self, record):
        """Proxy function to guarantee the base implementation of onPass is
        called.

        Args:
            record: The records.TestResultRecord object for the passed test
                    case.
        """
        test_name = record.test_name
        begin_time = logger.epochToLogLineTimestamp(record.begin_time)
        msg = record.details
        if msg:
            logging.info(msg)
        logging.info(RESULT_LINE_TEMPLATE, test_name, record.result)
        self.onPass(test_name, begin_time)

    def onPass(self, test_name, begin_time):
        """A function that is executed upon a test case passing.

        Implementation is optional.

        Args:
            test_name: Name of the test that triggered this function.
            begin_time: Logline format timestamp taken when the test started.
        """

    def _onSkip(self, record):
        """Proxy function to guarantee the base implementation of onSkip is
        called.

        Args:
            record: The records.TestResultRecord object for the skipped test
                    case.
        """
        test_name = record.test_name
        begin_time = logger.epochToLogLineTimestamp(record.begin_time)
        logging.info(RESULT_LINE_TEMPLATE, test_name, record.result)
        logging.info("Reason to skip: %s", record.details)
        self.onSkip(test_name, begin_time)

    def onSkip(self, test_name, begin_time):
        """A function that is executed upon a test case being skipped.

        Implementation is optional.

        Args:
            test_name: Name of the test that triggered this function.
            begin_time: Logline format timestamp taken when the test started.
        """

    def _onException(self, record):
        """Proxy function to guarantee the base implementation of onException
        is called.

        Args:
            record: The records.TestResultRecord object for the failed test
                    case.
        """
        test_name = record.test_name
        logging.exception(record.details)
        begin_time = logger.epochToLogLineTimestamp(record.begin_time)
        self.onException(test_name, begin_time)

    def onException(self, test_name, begin_time):
        """A function that is executed upon an unhandled exception from a test
        case.

        Implementation is optional.

        Args:
            test_name: Name of the test that triggered this function.
            begin_time: Logline format timestamp taken when the test started.
        """

    def _exec_procedure_func(self, func, tr_record):
        """Executes a procedure function like onPass, onFail etc.

        This function will alternate the 'Result' of the test's record if
        exceptions happened when executing the procedure function.

        This will let signals.TestAbortAll through so abortAll works in all
        procedure functions.

        Args:
            func: The procedure function to be executed.
            tr_record: The TestResultRecord object associated with the test
                       case executed.
        """
        try:
            func(tr_record)
        except signals.TestAbortAll:
            raise
        except Exception as e:
            logging.exception("Exception happened when executing %s for %s.",
                              func.__name__, self.currentTestName)
            tr_record.addError(func.__name__, e)

    def checkTestFilter(self, test_name):
        """Check whether a test should be filtered.

        If include filter is not empty, only tests in include filter will be
        executed. Exclude filter will only be check when include filter is
        not empty.

        Args:
            test_name: string, name of test

        Raises:
            TestSilent, when a test should be filtered.
        """
        if not keys.ConfigKeys.KEY_TEST_SUITE in self.user_params:
            return

        test_suite = self.user_params[keys.ConfigKeys.KEY_TEST_SUITE]

        if (test_suite[keys.ConfigKeys.KEY_INCLUDE_FILTER] and
                test_name in test_suite[keys.ConfigKeys.KEY_INCLUDE_FILTER]):
            raise TestSilent("Test case '%s' not in include filter." %
                             test_name)
        elif (test_suite[keys.ConfigKeys.KEY_EXCLUDE_FILTER] and
              test_name in test_suite[keys.ConfigKeys.KEY_EXCLUDE_FILTER]):
            raise TestSilent("Test case '%s' in exclude filter." % test_name)

    def execOneTest(self, test_name, test_func, args, **kwargs):
        """Executes one test case and update test results.

        Executes one test case, create a records.TestResultRecord object with
        the execution information, and add the record to the test class's test
        results.

        Args:
            test_name: Name of the test.
            test_func: The test function.
            args: A tuple of params.
            kwargs: Extra kwargs.
        """
        is_silenced = False
        tr_record = records.TestResultRecord(test_name, self.TAG)
        tr_record.testBegin()
        logging.info("%s %s", TEST_CASE_TOKEN, test_name)
        verdict = None
        try:
            self.checkTestFilter(test_name)
            ret = self._setUpTest(test_name)
            asserts.assertTrue(ret is not False,
                               "Setup for %s failed." % test_name)
            try:
                if args or kwargs:
                    verdict = test_func(*args, **kwargs)
                else:
                    verdict = test_func()
            finally:
                self._tearDownTest(test_name)
        except (signals.TestFailure, AssertionError) as e:
            tr_record.testFail(e)
            self._exec_procedure_func(self._onFail, tr_record)
        except signals.TestSkip as e:
            # Test skipped.
            tr_record.testSkip(e)
            self._exec_procedure_func(self._onSkip, tr_record)
        except (signals.TestAbortClass, signals.TestAbortAll) as e:
            # Abort signals, pass along.
            tr_record.testFail(e)
            raise e
        except signals.TestPass as e:
            # Explicit test pass.
            tr_record.testPass(e)
            self._exec_procedure_func(self._onPass, tr_record)
        except signals.TestSilent as e:
            # Suppress test reporting.
            is_silenced = True
            self.results.requested.remove(test_name)
        except Exception as e:
            # Exception happened during test.
            logging.exception(e)
            tr_record.testError(e)
            self._exec_procedure_func(self._onException, tr_record)
            self._exec_procedure_func(self._onFail, tr_record)
        else:
            # Keep supporting return False for now.
            # TODO(angli): Deprecate return False support.
            if verdict or (verdict is None):
                # Test passed.
                tr_record.testPass()
                self._exec_procedure_func(self._onPass, tr_record)
                return
            # Test failed because it didn't return True.
            # This should be removed eventually.
            tr_record.testFail()
            self._exec_procedure_func(self._onFail, tr_record)
        finally:
            if not is_silenced:
                self.results.addRecord(tr_record)

    def runGeneratedTests(self,
                          test_func,
                          settings,
                          args=None,
                          kwargs=None,
                          tag="",
                          name_func=None):
        """Runs generated test cases.

        Generated test cases are not written down as functions, but as a list
        of parameter sets. This way we reduce code repetition and improve
        test case scalability.

        Args:
            test_func: The common logic shared by all these generated test
                       cases. This function should take at least one argument,
                       which is a parameter set.
            settings: A list of strings representing parameter sets. These are
                      usually json strings that get loaded in the test_func.
            args: Iterable of additional position args to be passed to
                  test_func.
            kwargs: Dict of additional keyword args to be passed to test_func
            tag: Name of this group of generated test cases. Ignored if
                 name_func is provided and operates properly.
            name_func: A function that takes a test setting and generates a
                       proper test name. The test name should be shorter than
                       utils.MAX_FILENAME_LEN. Names over the limit will be
                       truncated.

        Returns:
            A list of settings that did not pass.
        """
        args = args or ()
        kwargs = kwargs or {}
        failed_settings = []
        for s in settings:
            test_name = "{} {}".format(tag, s)
            if name_func:
                try:
                    test_name = name_func(s, *args, **kwargs)
                except:
                    logging.exception(("Failed to get test name from "
                                       "test_func. Fall back to default %s"),
                                      test_name)
            self.results.requested.append(test_name)
            if len(test_name) > utils.MAX_FILENAME_LEN:
                test_name = test_name[:utils.MAX_FILENAME_LEN]
            previous_success_cnt = len(self.results.passed)
            self.execOneTest(test_name, test_func, (s, ) + args, **kwargs)
            if len(self.results.passed) - previous_success_cnt != 1:
                failed_settings.append(s)
        return failed_settings

    def _exec_func(self, func, *args):
        """Executes a function with exception safeguard.

        This will let signals.TestAbortAll through so abortAll works in all
        procedure functions.

        Args:
            func: Function to be executed.
            args: Arguments to be passed to the function.

        Returns:
            Whatever the function returns, or False if unhandled exception
            occured.
        """
        try:
            return func(*args)
        except signals.TestAbortAll:
            raise
        except:
            logging.exception("Exception happened when executing %s in %s.",
                              func.__name__, self.TAG)
            return False

    def _get_all_test_names(self):
        """Finds all the function names that match the test case naming
        convention in this class.

        Returns:
            A list of strings, each is a test case name.
        """
        test_names = []
        for name in dir(self):
            if name.startswith(STR_TEST) or name.startswith(STR_GENERATE):
                attr_func = getattr(self, name)
                if hasattr(attr_func, "__call__"):
                    test_names.append(name)
        return test_names

    def _get_test_funcs(self, test_names):
        """Obtain the actual functions of test cases based on test names.

        Args:
            test_names: A list of strings, each string is a test case name.

        Returns:
            A list of tuples of (string, function). String is the test case
            name, function is the actual test case function.

        Raises:
            errors.USERError is raised if the test name does not follow
            naming convention "test_*". This can only be caused by user input
            here.
        """
        test_funcs = []
        for test_name in test_names:
            if not hasattr(self, test_name):
                logging.warning("%s does not have test case %s.", self.TAG,
                                test_name)
            elif (test_name.startswith(STR_TEST) or
                  test_name.startswith(STR_GENERATE)):
                test_funcs.append((test_name, getattr(self, test_name)))
            else:
                msg = ("Test case name %s does not follow naming convention "
                       "test*, abort.") % test_name
                raise errors.USERError(msg)

        return test_funcs

    def run(self, test_names=None):
        """Runs test cases within a test class by the order they appear in the
        execution list.

        One of these test cases lists will be executed, shown here in priority
        order:
        1. The test_names list, which is passed from cmd line. Invalid names
           are guarded by cmd line arg parsing.
        2. The self.tests list defined in test class. Invalid names are
           ignored.
        3. All function that matches test case naming convention in the test
           class.

        Args:
            test_names: A list of string that are test case names requested in
                cmd line.

        Returns:
            The test results object of this class.
        """
        logging.info("==========> %s <==========", self.TAG)
        # Devise the actual test cases to run in the test class.
        if not test_names:
            if self.tests:
                # Specified by run list in class.
                test_names = list(self.tests)
            else:
                # No test case specified by user, execute all in the test class
                test_names = self._get_all_test_names()
        self.results.requested = [test_name for test_name in test_names
                                  if test_name.startswith(STR_TEST)]
        tests = self._get_test_funcs(test_names)

        # Setup for the class.
        try:
            if self._setUpClass() is False:
                raise signals.TestFailure("Failed to setup %s." % self.TAG)
        except Exception as e:
            logging.exception("Failed to setup %s.", self.TAG)
            self.results.failClass(self.TAG, e)
            self._exec_func(self._tearDownClass)
            return self.results
        # Run tests in order.
        try:
            for test_name, test_func in tests:
                if test_name.startswith(STR_GENERATE):
                    logging.info(
                        "Executing generated test trigger function '%s'",
                        test_name)
                    test_func()
                    logging.info("Finished '%s'", test_name)
                else:
                    self.execOneTest(test_name, test_func, None)
            return self.results
        except signals.TestAbortClass:
            logging.info("Received TestAbortClass signal")
            return self.results
        except signals.TestAbortAll as e:
            logging.info("Received TestAbortAll signal")
            # Piggy-back test results on this exception object so we don't lose
            # results from this test class.
            setattr(e, "results", self.results)
            raise e
        finally:
            self._exec_func(self._tearDownClass)
            logging.info("Summary for test class %s: %s", self.TAG,
                         self.results.summary())

    def cleanUp(self):
        """A function that is executed upon completion of all tests cases
        selected in the test class.

        This function should clean up objects initialized in the constructor by
        user.
        """
