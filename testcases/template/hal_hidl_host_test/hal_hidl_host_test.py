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

import logging

from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.precondition import precondition_utils


class HalHidlHostTest(base_test.BaseTestClass):
    """Base class to run a host-driver hidl hal test.

    Attributes:
        dut: AndroidDevice, the device under test as config
        shell: ShellMirrorObject, shell mirror
        TEST_HAL_SERVICES: a set of hal services accessed in the test.
    """
    TEST_HAL_SERVICES = set()

    def setUpClass(self):
        """Basic setup process for host-side hidl hal tests.

        Register default device and shell mirror, set permission, check whether
        the test satisfy the precondition requirement, prepare for test
        profiling and coverage measurement if enabled.
        """
        self.dut = self.registerController(android_device)[0]
        self.shell = self.dut.shell
        self.shell.Execute("setenforce 0")  # SELinux permissive mode

        # Testability check.
        if not precondition_utils.CanRunHidlHalTest(
                self, self.dut, self.shell, self.run_as_compliance_test):
            self._skip_all_testcases = True
            return

        # Initialization for coverage measurement.
        if self.coverage.enabled and self.coverage.global_coverage:
            self.coverage.InitializeDeviceCoverage(self.dut)
            if self.TEST_HAL_SERVICES:
                self.coverage.SetHalNames(self.TEST_HAL_SERVICES)

        # Enable profiling.
        if self.profiling.enabled:
            self.profiling.EnableVTSProfiling(self.shell)

    def tearDownClass(self):
        """Basic cleanup process for host-side hidl hal tests.

        If profiling is enabled for the test, collect the profiling data
        and disable profiling after the test is done.
        If coverage is enabled for the test, collect the coverage data and
        upload it to dashboard.
        """
        if self.coverage.enabled and self.coverage.global_coverage:
            self.coverage.SetCoverageData(dut=self.dut, isGlobal=True)

        if not self._skip_all_testcases and self.profiling.enabled:
            self.profiling.ProcessAndUploadTraceData()

    def setUp(self):
        """Setup process for each test case."""
        if self.profiling.enabled:
            self.profiling.EnableVTSProfiling(self.shell)

    def tearDown(self):
        """Cleanup process for each test case."""
        if self.profiling.enabled:
            self.profiling.ProcessTraceDataForTestCase(self.dut)
            self.profiling.DisableVTSProfiling(self.shell)

