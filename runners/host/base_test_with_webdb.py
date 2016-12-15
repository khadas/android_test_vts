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

import getpass
import logging
import os
import traceback
import time

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
from vts.utils.python.coverage import coverage_report

_ANDROID_DEVICE = "AndroidDevice"


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
    COVERAGE_SRC_FILES = "coverage_src_files"
    COVERAGE_ATTRIBUTE = "_gcov_coverage_data_dict"

    def __init__(self, configs):
        super(BaseTestWithWebDbClass, self).__init__(configs)

    def _setUpClass(self):
        """Proxy function to guarantee the base implementation of setUpClass
        is called.
        """
        self.getUserParams(opt_param_names=[self.USE_GAE_DB,
                                            self.COVERAGE_SRC_FILES])

        if getattr(self, self.USE_GAE_DB, False):
            logging.info("GAE-DB: turned on")
            self._report_msg = ReportMsg.TestReportMessage()
            self._report_msg.test = self.__class__.__name__
            self._report_msg.test_type = ReportMsg.VTS_HOST_DRIVEN_STRUCTURAL
            self._report_msg.start_timestamp = self.GetTimestamp()

        self._profile_msg = None
        self._profiling = {}
        setattr(self, self.COVERAGE_ATTRIBUTE, [])
        return super(BaseTestWithWebDbClass, self)._setUpClass()

    def _tearDownClass(self):
        if getattr(self, self.USE_GAE_DB, False):
            self._report_msg.end_timestamp = self.GetTimestamp()

            logging.info("_tearDownClass hook: start (username: %s)",
                         getpass.getuser())
            bt_client = bigtable_rest_client.HbaseRestClient(
                "result_%s" % self._report_msg.test)
            bt_client.CreateTable("test")

            self.getUserParams(opt_param_names=[keys.ConfigKeys.IKEY_BUILD])
            if getattr(self, keys.ConfigKeys.IKEY_BUILD, False):
                build = self.build
                if "build_id" in build:
                    self._report_msg.build_info.id = str(build["build_id"])

            self.SetDeviceInfo(self._report_msg)
            bt_client.PutRow(str(self._report_msg.start_timestamp),
                             "data", self._report_msg.SerializeToString())
            logging.info("_tearDownClass hook: result proto %s",
                         self._report_msg)
            if self._profile_msg:
                bt_client.CreateTable("profile")
                bt_client.PutRow(str(self._report_msg.start_timestamp),
                                 "data", self._profile_msg.SerializeToString())
                logging.info("_tearDownClass hook: profile proto %s",
                             self._profile_msg)
            logging.info("_tearDownClass hook: done")
        return super(BaseTestWithWebDbClass, self)._tearDownClass()

    def SetDeviceInfo(self, msg):
        """Sets device info to the given protobuf message, msg."""
        self.getUserParams(opt_param_names=[_ANDROID_DEVICE])
        dev_list = getattr(self, _ANDROID_DEVICE, False)
        if not dev_list or not isinstance(dev_list, list):
            logging.warn("attribute %s not found (available %s)",
                         _ANDROID_DEVICE,
                         self.user_params)
            return

        for device_spec in dev_list:
            dev_info = msg.device_info.add()
            for elem in [keys.ConfigKeys.IKEY_PRODUCT_TYPE,
                         keys.ConfigKeys.IKEY_PRODUCT_VARIANT,
                         keys.ConfigKeys.IKEY_BUILD_FLAVOR,
                         keys.ConfigKeys.IKEY_BUILD_ID,
                         keys.ConfigKeys.IKEY_BRANCH,
                         keys.ConfigKeys.IKEY_BUILD_ALIAS,
                         keys.ConfigKeys.IKEY_API_LEVEL,
                         keys.ConfigKeys.IKEY_SERIAL]:
                if elem in device_spec:
                    setattr(dev_info, elem, str(device_spec[elem]))

    def GetFunctionName(self):
        """Returns the caller's function name."""
        return traceback.extract_stack(None, 2)[0][2]

    def _setUpTest(self, test_name):
        """Proxy function to guarantee the base implementation of setUpTest is
        called.
        """
        if getattr(self, self.USE_GAE_DB, False):
            self._current_test_report_msg = self._report_msg.test_case.add()
            self._current_test_report_msg.name = test_name
            self._current_test_report_msg.start_timestamp = self.GetTimestamp()
        return super(BaseTestWithWebDbClass, self)._setUpTest(test_name)

    def _tearDownTest(self, test_name):
        """Proxy function to guarantee the base implementation of tearDownTest
        is called.
        """
        if getattr(self, self.USE_GAE_DB, False):
            self._current_test_report_msg.end_timestamp = self.GetTimestamp()
            if hasattr(self, self.COVERAGE_SRC_FILES):
                if not hasattr(self, keys.ConfigKeys.IKEY_DATA_FILE_PATH):
                    logging.warning("data_file_path not set. PATH=%s",
                                    os.environ["PATH"])
                else:
                    self.ProcessCoverageData()
            else:
                logging.info("coverage - no coverage src file specified")
        return super(BaseTestWithWebDbClass, self)._tearDownTest(test_name)

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
            False

        if not self._profile_msg:
            self._profile_msg = ReportMsg.TestReportMessage()
            self._profile_msg.test = self.__class__.__name__
        if name in self._profiling:
            logging.error("profiling point %s is already active.", name)
            return False
        self._profiling[name] = self._profile_msg.profiling.add()
        self._profiling[name].name = name
        self._profiling[name].start_timestamp = self.GetTimestamp()
        return True

    def StopProfiling(self, name):
        """Stops a profiling operation.

        Args:
            name: string, the name of a profiling point
        """
        if not getattr(self, self.USE_GAE_DB, False):
            logging.error("'use_gae_db' config is not True.")
            False

        if not self._profile_msg or name not in self._profiling:
            logging.error("profiling point %s is not active.", name)
            return False
        if self._profiling[name].end_timestamp:
            logging.error("profiling point %s already has data.", name)
            return False
        self._profiling[name].end_timestamp = self.GetTimestamp()
        return True

    def SetCoverageData(self, raw_coverage_data):
      """Sets the given coverage data to the class-level list attribute.

      In case of gcda, the file is always appended so the last one alone is
      sufficient for coverage visualization.

      Args:
          raw_coverage_data: a list of NativeCodeCoverageRawDataMessage.
      """
      logging.info("SetCoverageData %s", raw_coverage_data)
      setattr(self, self.COVERAGE_ATTRIBUTE, raw_coverage_data)

    def ProcessCoverageData(self):
        """Process reported coverage data and store the produced html(s).

        Returns:
            True if successful, False otherwise.
        """
        gcda_dict = getattr(
            self, self.COVERAGE_ATTRIBUTE, [])
        if not gcda_dict:
            logging.error("no coverage data found")
            return False
        for src_file in getattr(self, self.COVERAGE_SRC_FILES):
            src_file_name = str(src_file)
            logging.info("coverage - src file: %s", src_file_name)
            coverage = self._current_test_report_msg.coverage.add()

            coverage.file_name = src_file_name
            abs_path = os.path.join(self.data_file_path, src_file_name)
            src_file_content = None
            if not os.path.exists(abs_path):
                logging.error("couldn't find src file %s", abs_path)
                return False
            with open(abs_path, "rb") as f:
                src_file_content = f.read()

            if src_file_name.endswith(".c"):
                gcno_file_name = src_file_name.replace(".c", ".gcno")
                gcda_file_name = src_file_name.replace(".c", ".gcda")
            elif src_file_name.endswith(".cpp"):
                gcno_file_name = src_file_name.replace(".cpp", ".gcno")
                gcda_file_name = src_file_name.replace(".cpp", ".gcda")
            elif src_file_name.endswith(".cc"):
                gcno_file_name = src_file_name.replace(".cc", ".gcno")
                gcda_file_name = src_file_name.replace(".cc", ".gcda")
            else:
                logging.error("unsupported source file type %s",
                              src_file_name)
                return False

            abs_path = os.path.join(self.data_file_path, gcno_file_name)
            gcno_file_content = None
            if not os.path.exists(abs_path):
                logging.error("couldn't find gcno file %s", abs_path)
                return False
            with open(abs_path, "rb") as f:
                gcno_file_content = f.read()

            if gcda_dict:
                for file_path in gcda_dict:
                    # TODO: consider path and do exact matching
                    logging.info("check if %s in %s", gcda_file_name, file_path)
                    if file_path in gcda_file_name:
                        coverage.html = coverage_report.GenerateCoverageReport(
                            src_file_name, src_file_content,
                            gcno_file_content, gcda_dict[file_path])
                        logging.info("HTML Report: %s", coverage.html)
                        return True
