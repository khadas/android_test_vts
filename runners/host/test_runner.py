#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from future import standard_library
standard_library.install_aliases()

import copy
import importlib
import inspect
import logging
import os
import pkgutil
import sys

from vts.runners.host import keys
from vts.runners.host import logger
from vts.runners.host import records
from vts.runners.host import signals
from vts.runners.host import utils

class TestRunner(object):
    """The class that instantiates test classes, executes test cases, and
    report results.

    Attributes:
        self.test_run_info: A dictionary containing the information needed by
                            test classes for this test run, including params,
                            controllers, and other objects. All of these will
                            be passed to test classes.
        self.test_configs: A dictionary that is the original test configuration
                           passed in by user.
        self.id: A string that is the unique identifier of this test run.
        self.log_path: A string representing the path of the dir under which
                       all logs from this test run should be written.
        self.log: The logger object used throughout this test run.
        self.controller_registry: A dictionary that holds the controller
                                  objects used in a test run.
        self.controller_destructors: A dictionary that holds the controller
                                     distructors. Keys are controllers' names.
        self.test_classes: A dictionary where we can look up the test classes
                           by name to instantiate.
        self.run_list: A list of tuples specifying what tests to run.
        self.results: The test result object used to record the results of
                      this test run.
        self.running: A boolean signifies whether this test run is ongoing or
                      not.
    """
    def __init__(self, test_configs, run_list):
        self.test_run_info = {}
        self.test_configs = test_configs
        self.testbed_configs = self.test_configs[keys.Config.KEY_TESTBED.value]
        self.testbed_name = self.testbed_configs[keys.Config.KEY_TESTBED_NAME.value]
        start_time = logger.getLogFileTimestamp()
        self.id = "{}@{}".format(self.testbed_name, start_time)
        # log_path should be set before parsing configs.
        l_path = os.path.join(self.test_configs[keys.Config.KEP_LOG_PATH.value],
                              self.testbed_name,
                              start_time)
        self.log_path = os.path.abspath(l_path)
        logger.initiateTestLogger(self.log_path, self.id, self.testbed_name)
        self.log = logging.getLogger()
        self.controller_registry = {}
        self.controller_destructors = {}
        self.run_list = run_list
        self.results = records.TestResult()
        self.running = False

    def importTestModules(self, test_paths):
        """Imports test classes from test scripts.

        1. Locate all .py files under test paths.
        2. Import the .py files as modules.
        3. Find the module members that are test classes.
        4. Categorize the test classes by name.

        Args:
            test_paths: A list of directory paths where the test files reside.

        Returns:
            A dictionary where keys are test class name strings, values are
            actual test classes that can be instantiated.
        """
        def is_testfile_name(name, ext):
            if ext == ".py":
                if name.endswith("Test") or name.endswith("_test"):
                    return True
            return False
        file_list = utils.find_files(test_paths, is_testfile_name)
        test_classes = {}
        for path, name, _ in file_list:
            sys.path.append(path)
            try:
                module = importlib.import_module(name)
            except:
                for test_cls_name, _ in self.run_list:
                    alt_name = name.replace('_', '').lower()
                    alt_cls_name = test_cls_name.lower()
                    # Only block if a test class on the run list causes an
                    # import error. We need to check against both naming
                    # conventions: AaaBbb and aaa_bbb.
                    if name == test_cls_name or alt_name == alt_cls_name:
                        msg = ("Encountered error importing test class %s, "
                               "abort.") % test_cls_name
                        # This exception is logged here to help with debugging
                        # under py2, because "raise X from Y" syntax is only
                        # supported under py3.
                        self.log.exception(msg)
                        raise USERError(msg)
                continue
            for member_name in dir(module):
                if not member_name.startswith("__"):
                    if member_name.endswith("Test"):
                        test_class = getattr(module, member_name)
                        if inspect.isclass(test_class):
                            test_classes[member_name] = test_class
        return test_classes

    @staticmethod
    def verifyControllerModule(module):
        """Verifies a module object follows the required interface for
        controllers.

        Args:
            module: An object that is a controller module. This is usually
                    imported with import statements or loaded by importlib.

        Raises:
            ControllerError is raised if the module does not match the vts.runners.host
            controller interface, or one of the required members is null.
        """
        required_attributes = ("create",
                               "destroy",
                               "VTS_CONTROLLER_CONFIG_NAME")
        for attr in required_attributes:
            if not hasattr(module, attr):
                raise signals.ControllerError(("Module %s missing required "
                    "controller module attribute %s.") % (module.__name__,
                                                          attr))
            if not getattr(module, attr):
                raise signals.ControllerError(("Controller interface %s in %s "
                    "cannot be null.") % (attr, module.__name__))

    def registerController(self, module):
        """Registers a controller module for a test run.

        This declares a controller dependency of this test class. If the target
        module exists and matches the controller interface, the controller
        module will be instantiated with corresponding configs in the test
        config file. The module should be imported first.

        Params:
            module: A module that follows the controller module interface.

        Returns:
            A list of controller objects instantiated from controller_module.

        Raises:
            ControllerError is raised if no corresponding config can be found,
            or if the controller module has already been registered.
        """
        TestRunner.verifyControllerModule(module)
        module_ref_name = module.__name__.split('.')[-1]
        if module_ref_name in self.controller_registry:
            raise signals.ControllerError(("Controller module %s has already "
                                           "been registered. It can not be "
                                           "registered again."
                                           ) % module_ref_name)
        # Create controller objects.
        create = module.create
        module_config_name = module.VTS_CONTROLLER_CONFIG_NAME
        if module_config_name not in self.testbed_configs:
            raise signals.ControllerError(("No corresponding config found for"
                                           " %s") % module_config_name)
        try:
            # Make a deep copy of the config to pass to the controller module,
            # in case the controller module modifies the config internally.
            original_config = self.testbed_configs[module_config_name]
            controller_config = copy.deepcopy(original_config)
            objects = create(controller_config, self.log)
        except:
            self.log.exception(("Failed to initialize objects for controller "
                                "%s, abort!"), module_config_name)
            raise
        if not isinstance(objects, list):
            raise ControllerError(("Controller module %s did not return a list"
                                   " of objects, abort.") % module_ref_name)
        self.controller_registry[module_ref_name] = objects
        self.log.debug("Found %d objects for controller %s", len(objects),
                       module_config_name)
        destroy_func = module.destroy
        self.controller_destructors[module_ref_name] = destroy_func
        return objects

    def unregisterControllers(self):
        """Destroy controller objects and clear internal registry.

        This will be called at the end of each TestRunner.run call.
        """
        for name, destroy in self.controller_destructors.items():
            try:
                self.log.debug("Destroying %s.", name)
                destroy(self.controller_registry[name])
            except:
                self.log.exception("Exception occurred destroying %s.", name)
        self.controller_registry = {}
        self.controller_destructors = {}

    def parseTestConfig(self, test_configs):
        """Parses the test configuration and unpacks objects and parameters
        into a dictionary to be passed to test classes.

        Args:
            test_configs: A json object representing the test configurations.
        """
        self.test_run_info[keys.Config.IKEY_TESTBED_NAME.value] = self.testbed_name
        # Unpack other params.
        self.test_run_info["registerController"] = self.registerController
        self.test_run_info[keys.Config.IKEY_LOG_PATH.value] = self.log_path
        user_param_pairs = []
        for item in test_configs.items():
            if item[0] not in keys.Config.RESERVED_KEYS.value:
                user_param_pairs.append(item)
        self.test_run_info[keys.Config.IKEY_USER_PARAM.value] = dict(user_param_pairs)

    def runTestClass(self, test_cls_name, test_cases=None):
        """Instantiates and executes a test class.

        If test_cases is None, the test cases listed by self.tests will be
        executed instead. If self.tests is empty as well, no test case in this
        test class will be executed.

        Args:
            test_cls_name: Name of the test class to execute.
            test_cases: List of test case names to execute within the class.

        Returns:
            A tuple, with the number of cases passed at index 0, and the total
            number of test cases at index 1.
        """
        try:
            test_cls = self.test_classes[test_cls_name]
        except KeyError:
            raise USERError(("Unable to locate class %s in any of the test "
                "paths specified.") % test_cls_name)

        with test_cls(self.test_run_info) as test_cls_instance:
            try:
                cls_result = test_cls_instance.run(test_cases)
                self.results += cls_result
            except signals.TestAbortAll as e:
                self.results += e.results
                raise e

    def run(self):
        """Executes test cases.

        This will instantiate controller and test classes, and execute test
        classes. This can be called multiple times to repeatly execute the
        requested test cases.

        A call to TestRunner.stop should eventually happen to conclude the life
        cycle of a TestRunner.
        """
        if not self.running:
            self.running = True
        # Initialize controller objects and pack appropriate objects/params
        # to be passed to test class.
        self.parseTestConfig(self.test_configs)
        t_configs = self.test_configs[keys.Config.KEY_TEST_PATHS.value]
        self.test_classes = self.importTestModules(t_configs)
        self.log.debug("Executing run list %s.", self.run_list)
        try:
            for test_cls_name, test_case_names in self.run_list:
                if not self.running:
                    break
                if test_case_names:
                    self.log.debug("Executing test cases %s in test class %s.",
                                   test_case_names,
                                   test_cls_name)
                else:
                    self.log.debug("Executing test class %s", test_cls_name)
                try:
                    self.runTestClass(test_cls_name, test_case_names)
                except signals.TestAbortAll as e:
                    self.log.warning(("Abort all subsequent test classes. Reason: "
                                      "%s"), e)
                    raise
        finally:
            self.unregisterControllers()

    def stop(self):
        """Releases resources from test run. Should always be called after
        TestRunner.run finishes.

        This function concludes a test run and writes out a test report.
        """
        if self.running:
            msg = "\nSummary for test run %s: %s\n" % (self.id,
                self.results.summary())
            self._writeResultsJsonString()
            self.log.info(msg.strip())
            logger.killTestLogger(self.log)
            self.running = False

    def _writeResultsJsonString(self):
        """Writes out a json file with the test result info for easy parsing.
        """
        path = os.path.join(self.log_path, "test_run_summary.json")
        with open(path, 'w') as f:
            f.write(self.results.jsonString())
