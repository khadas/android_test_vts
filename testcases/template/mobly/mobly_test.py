#
# Copyright (C) 2017 The Android Open Source Project
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

import json
import logging
import os
import time
import types

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import records
from vts.runners.host import test_runner
from vts.utils.python.io import capture_printout
from vts.utils.python.io import file_util

from mobly import test_runner as mobly_test_runner


LIST_TEST_OUTPUT_START = '==========> '
LIST_TEST_OUTPUT_END = ' <=========='
# Temp directory inside python log path. The name is required to be
# the set value for tradefed to skip reading contents as logs.
TEMP_DIR_NAME = 'temp'
CONFIG_FILE_NAME = 'test_config.yaml'
MOBLY_RESULT_FILE_NAME = 'test_run_summary.json'

MOBLY_CONFIG_TEXT = '''TestBeds:
  - Name: {module_name}
    Controllers:
        AndroidDevice:
          - serial: {serial1}
          - serial: {serial2}

MoblyParams:
    LogPath: {log_path}
'''

#TODO(yuexima):
# 1. make DEVICES_REQUIRED configurable
# 2. add include filter function
DEVICES_REQUIRED = 2


class MoblyTest(base_test.BaseTestClass):
    '''Template class for running mobly test cases.

    Attributes:
        mobly_dir: string, mobly test temp directory for mobly runner
        mobly_config_file_path: string, mobly test config file path
    '''
    def setUpClass(self):
        asserts.assertEqual(
            len(self.android_devices), DEVICES_REQUIRED,
            'Exactly %s devices are required for this test.' % DEVICES_REQUIRED
        )

        for ad in self.android_devices:
            logging.info('Android device serial: %s' % ad.serial)

        logging.info('Test cases: %s' % self.ListTestCases())

        self.mobly_dir = os.path.join(logging.log_path, TEMP_DIR_NAME,
                                      'mobly', str(time.time()))

        file_util.Makedirs(self.mobly_dir)

        logging.info('mobly log path: %s' % self.mobly_dir)

    def tearDownClass(self):
        ''' Clear the mobly directory.'''
        file_util.Rmdirs(self.mobly_dir, ignore_errors=True)

    def PrepareConfigFile(self):
        '''Prepare mobly config file for running test.'''
        self.mobly_config_file_path = os.path.join(self.mobly_dir,
                                                   CONFIG_FILE_NAME)
        config_test = MOBLY_CONFIG_TEXT.format(
              module_name=self.test_module_name,
              serial1=self.android_devices[0].serial,
              serial2=self.android_devices[1].serial,
              log_path=self.mobly_dir
        )
        with open(self.mobly_config_file_path, 'w') as f:
            f.write(config_test)

    def ListTestCases(self):
        '''List test cases.

        Returns:
            List of string, test names.
        '''
        classes = mobly_test_runner._find_test_class()

        with capture_printout.CaptureStdout() as output:
            mobly_test_runner._print_test_names(classes)

        test_names = []

        for line in output:
            if (not line.startswith(LIST_TEST_OUTPUT_START)
                and line.endswith(LIST_TEST_OUTPUT_END)):
                test_names.append(line)

        return test_names

    def RunMoblyModule(self):
        '''Execute mobly test module.'''
        # Because mobly and vts uses a similar runner, both will modify
        # log_path from python logging. The following step is to preserve
        # log path after mobly test finishes.

        # An alternative way is to start a new python process through shell
        # command. In that case, test print out needs to be piped.
        # This will also help avoid log overlapping

        logger = logging.getLogger()
        logger_path = logger.log_path
        logging_path = logging.log_path

        try:
            mobly_test_runner.main(argv=['-c', self.mobly_config_file_path])
        finally:
            logger.log_path = logger_path
            logging.log_path = logging_path

    def GetMoblyResults(self):
        '''Get mobly module run results and put in vts results.'''
        #TODO(yuexima): currently this only support one mobly module per vts
        # module. If multiple mobly module per vts module is needed in the
        # future, here is where it should be changed.
        path = file_util.FindFile(self.mobly_dir, MOBLY_RESULT_FILE_NAME)

        if not path:
            logging.error('No results are generated from mobly tests')
            return

        logging.info('Mobly test result path: %s', path)

        with open(path, 'r') as f:
            mobly_summary = json.load(f)

        mobly_results = mobly_summary['Results']
        for result in mobly_results:
            logging.info('Adding result for %s' % result[records.TestResultEnums.RECORD_NAME])
            record = records.TestResultRecord(result[records.TestResultEnums.RECORD_NAME])
            record.test_class = result[records.TestResultEnums.RECORD_CLASS]
            record.begin_time = result[records.TestResultEnums.RECORD_BEGIN_TIME]
            record.end_time = result[records.TestResultEnums.RECORD_END_TIME]
            record.result = result[records.TestResultEnums.RECORD_RESULT]
            record.uid = result[records.TestResultEnums.RECORD_UID]
            record.extras = result[records.TestResultEnums.RECORD_EXTRAS]
            record.details = result[records.TestResultEnums.RECORD_DETAILS]
            record.extra_errors = result[records.TestResultEnums.RECORD_EXTRA_ERRORS]

            self.results.addRecord(record)

    def generateAllTests(self):
        '''Run the mobly test module and parse results.'''
        #TODO(yuexima): report test names

        self.PrepareConfigFile()
        self.RunMoblyModule()
        #TODO(yuexima): check whether DEBUG logs from mobly run are included
        self.GetMoblyResults()


if __name__ == "__main__":
    test_runner.main()