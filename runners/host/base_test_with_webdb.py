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

import getpass
import io
import logging
import os
import socket
import traceback
import time
import xml.etree.ElementTree as ET
import zipfile

from google.protobuf.internal.containers import RepeatedCompositeFieldContainer

from vts.proto import VtsReportMessage_pb2 as ReportMsg
from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import errors
from vts.runners.host import keys
from vts.runners.host import logger
from vts.runners.host import records
from vts.runners.host import signals
from vts.runners.host import utils

from vts.utils.app_engine import bigtable_rest_client
from vts.utils.python.build.api import artifact_fetcher
from vts.utils.python.coverage import coverage_utils
from vts.utils.python.profiling import profiling_utils

_ANDROID_DEVICE = "AndroidDevice"
_MAX = "max"
_MIN = "min"
_AVG = "avg"


class BaseTestWithWebDbClass(base_test.BaseTestClass):
    """Base class with Web DB interface for test classes to inherit from.

    Attributes:
        tests: A list of strings, each representing a test case name.
        TAG: A string used to refer to a test class. Default is the test class
             name.
        results: A records.TestResult object for aggregating test results from
                 the execution of test cases.
        currentTestName: A string that's the name of the test case currently
                           being executed. If no test is executing, this should
                           be None.
        _report_msg: TestReportMessage, to store the test result to a GAE-side bigtable.
        _current_test_report_msg: a proto message keeping the information of a
                                  current test case.
        _report_msg: TestReportMessage, to store the profiling result to
                     a GAE-side bigtable.
        _profiling: a dict containing the current profiling information.
    """
    USE_GAE_DB = "use_gae_db"
    COVERAGE = "coverage"
    MODULES = "modules"
    GIT_PROJECT_NAME = "git_project_name"
    GIT_PROJECT_PATH = "git_project_path"
    SERVICE_JSON_PATH = "service_key_json_path"
    COVERAGE_ZIP = "coverage_zip"
    REVISION = "revision"
    STATUS_TABLE = "vts_status_table"
    BIGTABLE_BASE_URL = "bigtable_base_url"
    BRANCH = "master"
    VTS_PROFILING_TRACING_PATH = "profiling_trace_path"

    def __init__(self, configs):
        super(BaseTestWithWebDbClass, self).__init__(configs)

    def _setUpClass(self):
        """Proxy function to guarantee the base implementation of setUpClass
        is called.
        """
        self.getUserParams(opt_param_names=[
            self.USE_GAE_DB, self.BIGTABLE_BASE_URL, self.MODULES,
            self.GIT_PROJECT_NAME, self.GIT_PROJECT_PATH,
            self.SERVICE_JSON_PATH, keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.KEY_TESTBED_NAME, self.VTS_PROFILING_TRACING_PATH
        ])

        self.enable_profiling = self.getUserParam(
            keys.ConfigKeys.IKEY_ENABLE_PROFILING, default_value=False)

        if getattr(self, self.USE_GAE_DB, False):
            logging.info("GAE-DB: turned on")
            self._report_msg = ReportMsg.TestReportMessage()
            test_module_name = self.__class__.__name__
            if hasattr(self, keys.ConfigKeys.KEY_TESTBED_NAME):
                user_specified_test_name = getattr(
                    self, keys.ConfigKeys.KEY_TESTBED_NAME, None)
                if user_specified_test_name:
                    test_module_name = str(user_specified_test_name)
                else:
                    logging.warn("%s field = %s",
                                 keys.ConfigKeys.KEY_TESTBED_NAME,
                                 user_specified_test_name)
            else:
                logging.warn("%s not defined in the given test config",
                             keys.ConfigKeys.KEY_TESTBED_NAME)
            logging.info("Test module name: %s", test_module_name)
            self._report_msg.test = test_module_name
            self._report_msg.test_type = ReportMsg.VTS_HOST_DRIVEN_STRUCTURAL
            self._report_msg.start_timestamp = self.GetTimestamp()
            self._report_msg.host_info.hostname = socket.gethostname()
            self.SetDeviceInfo(self._report_msg)
            self.InitializeCoverage()
        self._profiling = {}
        return super(BaseTestWithWebDbClass, self)._setUpClass()

    def _tearDownClass(self):
        """Calls sub-class's tearDownClass first and then uploads to web DB."""
        result = super(BaseTestWithWebDbClass, self)._tearDownClass()
        if (getattr(self, self.USE_GAE_DB, False) and
                getattr(self, self.BIGTABLE_BASE_URL, "")):
            # Handle case when runner fails, tests aren't executed
            if (self.results.executed and
                    self.results.executed[-1].test_name == "setup_class"):
                # Test failed during setup, all tests were not executed
                start_index = 0
            else:
                # Runner was aborted. Remaining tests weren't executed
                start_index = len(self.results.executed)

            for test in self.results.requested[start_index:]:
                msg = self._report_msg.test_case.add()
                msg.name = test
                msg.start_timestamp = self.GetTimestamp()
                msg.end_timestamp = msg.start_timestamp
                msg.test_result = ReportMsg.TEST_CASE_RESULT_FAIL

            self._report_msg.end_timestamp = self.GetTimestamp()

            logging.info("_tearDownClass hook: start (username: %s)",
                         getpass.getuser())

            bt_url = getattr(self, self.BIGTABLE_BASE_URL)
            bt_client = bigtable_rest_client.HbaseRestClient(
                "result_%s" % self._report_msg.test, bt_url)
            bt_client.CreateTable("test")

            self.getUserParams(opt_param_names=[keys.ConfigKeys.IKEY_BUILD])
            if getattr(self, keys.ConfigKeys.IKEY_BUILD, False):
                build = self.build
                if keys.ConfigKeys.IKEY_BUILD_ID in build:
                    build_id = str(build[keys.ConfigKeys.IKEY_BUILD_ID])
                    self._report_msg.build_info.id = build_id

            bt_client.PutRow(
                str(self._report_msg.start_timestamp), "data",
                self._report_msg.SerializeToString())

            logging.info("_tearDownClass hook: report msg proto %s",
                         self._report_msg)

            bt_client = bigtable_rest_client.HbaseRestClient(self.STATUS_TABLE,
                                                             bt_url)
            bt_client.CreateTable("status")

            bt_client.PutRow("result_%s" % self._report_msg.test,
                             "upload_timestamp",
                             str(self._report_msg.start_timestamp))

            logging.info("_tearDownClass hook: status upload time stamp %s",
                         str(self._report_msg.start_timestamp))

            logging.info("_tearDownClass hook: done")
        else:
            logging.info("_tearDownClass hook: missing USE_GAE_DB and/or "
                         "BIGTABLE_BASE_URL. Web uploading disabled.")
        return result

    def SetDeviceInfo(self, msg):
        """Sets device info to the given protobuf message, msg."""
        self.getUserParams(opt_param_names=[_ANDROID_DEVICE])
        dev_list = getattr(self, _ANDROID_DEVICE, False)
        if not dev_list or not isinstance(dev_list, list):
            logging.warn("attribute %s not found (available %s)",
                         _ANDROID_DEVICE, self.user_params)
            return

        for device_spec in dev_list:
            dev_info = msg.device_info.add()
            for elem in [
                    keys.ConfigKeys.IKEY_PRODUCT_TYPE,
                    keys.ConfigKeys.IKEY_PRODUCT_VARIANT,
                    keys.ConfigKeys.IKEY_BUILD_FLAVOR,
                    keys.ConfigKeys.IKEY_BUILD_ID, keys.ConfigKeys.IKEY_BRANCH,
                    keys.ConfigKeys.IKEY_BUILD_ALIAS,
                    keys.ConfigKeys.IKEY_API_LEVEL, keys.ConfigKeys.IKEY_SERIAL
            ]:
                if elem in device_spec:
                    setattr(dev_info, elem, str(device_spec[elem]))
            # TODO: get abi information differently for multi-device support.
            if hasattr(self, keys.ConfigKeys.IKEY_ABI_NAME):
                setattr(dev_info, keys.ConfigKeys.IKEY_ABI_NAME,
                        str(getattr(self, keys.ConfigKeys.IKEY_ABI_NAME)))
            if hasattr(self, keys.ConfigKeys.IKEY_ABI_BITNESS):
                setattr(dev_info, keys.ConfigKeys.IKEY_ABI_BITNESS,
                        str(getattr(self, keys.ConfigKeys.IKEY_ABI_BITNESS)))

    def GetFunctionName(self):
        """Returns the caller's function name."""
        return traceback.extract_stack(None, 2)[0][2]

    def _testEntry(self, test_name):
        """Internal function to be called upon entry of a test case."""
        if getattr(self, self.USE_GAE_DB, False):
            self._current_test_report_msg = self._report_msg.test_case.add()
            self._current_test_report_msg.name = test_name
            self._current_test_report_msg.start_timestamp = self.GetTimestamp()
        return super(BaseTestWithWebDbClass, self)._testEntry(test_name)

    def _testExit(self, test_name):
        """Proxy function to guarantee the base implementation of tearDownTest
        is called.
        """
        if getattr(self, self.USE_GAE_DB, False):
            if self._current_test_report_msg:
                self._current_test_report_msg.end_timestamp = self.GetTimestamp(
                )
            else:
                logging.info(
                    "test result of '%s' is empty and will not be uploaded.",
                    test_name)
        return super(BaseTestWithWebDbClass, self)._testExit(test_name)

    def _onFail(self, record):
        """Proxy function to guarantee the base implementation of onFail is
        called.

        Args:
            record: The records.TestResultRecord object for the failed test
                    case.
        """
        if getattr(self, self.USE_GAE_DB, False):
            self._current_test_report_msg.test_result = ReportMsg.TEST_CASE_RESULT_FAIL
        return super(BaseTestWithWebDbClass, self)._onFail(record)

    def _onPass(self, record):
        """Proxy function to guarantee the base implementation of onPass is
        called.

        Args:
            record: The records.TestResultRecord object for the passed test
                    case.
        """
        if getattr(self, self.USE_GAE_DB, False):
            self._current_test_report_msg.test_result = ReportMsg.TEST_CASE_RESULT_PASS
        return super(BaseTestWithWebDbClass, self)._onPass(record)

    def _onSkip(self, record):
        """Proxy function to guarantee the base implementation of onSkip is
        called.

        Args:
            record: The records.TestResultRecord object for the skipped test
                    case.
        """
        if getattr(self, self.USE_GAE_DB, False):
            self._current_test_report_msg.test_result = ReportMsg.TEST_CASE_RESULT_SKIP
        return super(BaseTestWithWebDbClass, self)._onSkip(record)

    def _onSilent(self, record):
        """Proxy function to guarantee the base implementation of onSilent is
        called.

        Args:
            record: The records.TestResultRecord object for the skipped test
                    case.
        """
        if getattr(self, self.USE_GAE_DB, False):
            self._report_msg.test_case.remove(self._current_test_report_msg)
            self._current_test_report_msg = None
        return super(BaseTestWithWebDbClass, self)._onSilent(record)

    def _onException(self, record):
        """Proxy function to guarantee the base implementation of onException
        is called.

        Args:
            record: The records.TestResultRecord object for the failed test
                    case.
        """
        if getattr(self, self.USE_GAE_DB, False):
            self._current_test_report_msg.test_result = ReportMsg.TEST_CASE_RESULT_EXCEPTION
        return super(BaseTestWithWebDbClass, self)._onException(record)

    def GetTimestamp(self):
        """Returns the current UTC time (unit: microseconds)."""
        return int(time.time() * 1000000)

    def StartProfiling(self, name):
        """Starts a profiling operation.

        Args:
            name: string, the name of a profiling point

        Returns:
            True if successful, False otherwise
        """
        if not getattr(self, self.USE_GAE_DB, False):
            logging.error("'use_gae_db' config is not True.")
            return False

        if name in self._profiling:
            logging.error("profiling point %s is already active.", name)
            return False
        self._profiling[name] = self._report_msg.profiling.add()
        self._profiling[name].name = name
        self._profiling[name].type = ReportMsg.VTS_PROFILING_TYPE_TIMESTAMP
        self._profiling[name].start_timestamp = self.GetTimestamp()
        return True

    def StopProfiling(self, name):
        """Stops a profiling operation.

        Args:
            name: string, the name of a profiling point
        """
        if not getattr(self, self.USE_GAE_DB, False):
            logging.error("'use_gae_db' config is not True.")
            return False

        if name not in self._profiling:
            logging.error("profiling point %s is not active.", name)
            return False
        if self._profiling[name].end_timestamp:
            logging.error("profiling point %s already has data.", name)
            return False
        self._profiling[name].end_timestamp = self.GetTimestamp()
        return True

    def AddProfilingDataLabeledVector(self,
                                      name,
                                      labels,
                                      values,
                                      x_axis_label="x-axis",
                                      y_axis_label="y-axis"):
        """Adds the profiling data in order to upload to the web DB.

        Args:
            name: string, profiling point name.
            labels: a list of labels.
            values: a list of values.
            x-axis_label: string, the x-axis label title for a graph plot.
            y-axis_label: string, the y-axis label title for a graph plot.
        """
        if not getattr(self, self.USE_GAE_DB, False):
            logging.error("'use_gae_db' config is not True.")
            return False

        if name in self._profiling:
            logging.error("profiling point %s is already active.", name)
            return False

        self._profiling[name] = self._report_msg.profiling.add()
        self._profiling[name].name = name
        self._profiling[
            name].type = ReportMsg.VTS_PROFILING_TYPE_LABELED_VECTOR
        for label, value in zip(labels, values):
            self._profiling[name].label.append(label)
            self._profiling[name].value.append(value)
        self._profiling[name].x_axis_label = x_axis_label
        self._profiling[name].y_axis_label = y_axis_label

    def AddProfilingDataLabeledPoint(self, name, value):
        """Adds labeled point type profiling data for uploading to the web DB.

        Args:
            name: string, profiling point name.
            value: int, the value.
        """
        if not getattr(self, self.USE_GAE_DB, False):
            logging.error("'use_gae_db' config is not True.")
            return False

        if name in self._profiling:
            logging.error("profiling point %s is already active.", name)
            return False
        self._profiling[name] = self._report_msg.profiling.add()
        self._profiling[name].name = name
        self._profiling[name].type = ReportMsg.VTS_PROFILING_TYPE_TIMESTAMP
        self._profiling[name].start_timestamp = 0
        self._profiling[name].end_timestamp = value
        return True

    def InitializeCoverage(self):
        """Initializes the test for coverage instrumentation.

        Reads required user params and downloads build artifacts from the build
        server (gcno zip file and git revision dictionary).

        Returns:
            True if test is ready for coverage instrumentation, False otherwise.
        """
        setattr(self, self.COVERAGE, False)
        if len(self._report_msg.device_info) == 0:
            logging.error("could not read device info")
            return False

        # Use first device info to get product, flavor, and ID
        # TODO: support multi-device builds
        device_spec = self._report_msg.device_info[0]
        build_flavor = getattr(device_spec, keys.ConfigKeys.IKEY_BUILD_FLAVOR,
                               None)
        product = getattr(device_spec, keys.ConfigKeys.IKEY_PRODUCT_VARIANT,
                          None)
        device_build_id = getattr(device_spec, keys.ConfigKeys.IKEY_BUILD_ID,
                                  None)

        if not build_flavor or not product or not device_build_id:
            logging.error("Could not read device information.")
            return False
        build_flavor += "_coverage"

        # Get service json path
        service_json_path = getattr(self, self.SERVICE_JSON_PATH, False)
        if not service_json_path:
            logging.error("couldn't find service json path")
            return False

        # Get project name
        project_name = getattr(self, self.GIT_PROJECT_NAME, False)
        if not project_name:
            logging.error("couldn't find git project name")
            return False

        # Get project path
        project_path = getattr(self, self.GIT_PROJECT_PATH, False)
        if not project_path:
            logging.error("couldn't find project path")
            return False

        # Instantiate build client
        try:
            build_client = artifact_fetcher.AndroidBuildClient(
                service_json_path)
        except Exception:
            logging.error("Invalid service JSON file %s", service_json_path)
            return False

        # Fetch repo dictionary
        try:
            repos = build_client.GetRepoDictionary(self.BRANCH, build_flavor,
                                                   device_build_id)
        except:
            logging.error("Could not read build info for branch %s, " +
                          "target %s, id: %s", self.BRANCH, build_flavor,
                          device_build_id)
            return False

        # Get revision (commit ID) from manifest
        if project_name not in repos:
            logging.error("Could not find project %s in repo dictionary",
                          project_name)
            return False
        revision = str(repos[project_name])

        # Fetch coverage zip
        try:
            cov_zip = io.BytesIO(
                build_client.GetCoverage("master", build_flavor,
                                         device_build_id, product))
            cov_zip = zipfile.ZipFile(cov_zip)
        except:
            logging.error("Could not read coverage zip for branch %s, " +
                          "target %s, id: %s, product: %s" %
                          (self.BRANCH, build_flavor, device_build_id, product
                           ))
            return False
        setattr(self, self.COVERAGE_ZIP, cov_zip)
        setattr(self, self.REVISION, revision)
        setattr(self, self.COVERAGE, True)
        return True

    def SetCoverageData(self, coverage_data, isGlobal=False):
        """Sets and processes coverage data.

        Organizes coverage data and processes it into a coverage report in the
        current test case

        Args:
            coverage_data may be either:
                          (1) a dict where gcda name is the key and binary
                              content is the value, or
                          (2) a list of NativeCodeCoverageRawDataMessage objects
            isGlobal: True if the coverage data is for the entire test, False if
                      if the coverage data is just for the current test case.

        Returns:
            True if the coverage data is successfully processed, False otherwise
        """
        if not getattr(self, self.COVERAGE, False):
            logging.info("coverage disabled")
            return False

        if not coverage_data:
            logging.info("SetCoverageData: empty coverage data")
            return False

        if isinstance(coverage_data, RepeatedCompositeFieldContainer):
            gcda_dict = {}
            for coverage_msg in coverage_data:
                gcda_dict[coverage_msg.file_path] = coverage_msg.gcda
        elif isinstance(coverage_data, dict):
            gcda_dict = coverage_data
        else:
            logging.error("unexpected coverage_data type: %s",
                          str(type(coverage_data)))
            return False
        logging.info("coverage file paths %s", str([fp for fp in gcda_dict]))
        report_msg = self._report_msg if isGlobal else self._current_test_report_msg

        try:
            cov_zip = getattr(self, self.COVERAGE_ZIP)
            modules = getattr(self, self.MODULES)
            project_name = getattr(self, self.GIT_PROJECT_NAME)
            project_path = getattr(self, self.GIT_PROJECT_PATH)
            revision = getattr(self, self.REVISION)
        except AttributeError as e:
            logging.error("attributes not found %s", str(e))
            return False

        coverage_utils.ProcessCoverageData(report_msg, cov_zip, modules,
                                           gcda_dict, project_name,
                                           project_path, revision)
        return True

    def ProcessAndUploadTraceData(self, dut, profiling_trace_path):
        """Pulls the generated profiling data to the host, parses the data to
        get the max/min/avg latency of each API call and uploads these latency
        metrics to webdb.

        Args:
            dut: the registered device.
            porfiling_trace_path: path to store the profiling data on host.
        """
        trace_files = profiling_utils.GetTraceFiles(dut, profiling_trace_path)
        for file in trace_files:
            logging.info("parsing trace file: %s.", file)
            data = profiling_utils.ParseTraceData(file)
            for tag in [_MAX, _MIN, _AVG]:
                self.AddProfilingDataLabeledVector(
                    data.name + "_" + tag,
                    data.labels,
                    data.values[tag],
                    x_axis_label="API name",
                    y_axis_label="API processing latency (nano secs)")
