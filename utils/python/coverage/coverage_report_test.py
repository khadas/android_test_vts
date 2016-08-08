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

import os
import unittest

from vts.utils.python.coverage import coverage_report


class CoverageReportTest(unittest.TestCase):
    """End-to-end tests for CoverageReport of vts.utils.python.coverage.
    """

    def testSampleFile(self):
        """Verifies that sample file coverage is generated correctly.
        """
        root_dir = os.path.join(
            os.getenv('ANDROID_BUILD_TOP'),
            'test/vts/utils/python/coverage/testdata/')
        src_path = os.path.join(root_dir, 'sample.c')
        with open(src_path, 'r') as file:
            src_content = file.read()
        gcno_path = os.path.join(root_dir, 'sample.gcno')
        with open(gcno_path, 'rb') as file:
            gcno_content = file.read()
        gcda_path = os.path.join(root_dir, 'sample.gcda')
        with open(gcda_path, 'rb') as file:
            gcda_content = file.read()
        html_path = os.path.join(root_dir, 'sample_coverage.html')
        with open(html_path, 'r') as file:
            html_content = file.read()
        html_generated = coverage_report.GenerateCoverageReport('sample.c', src_content,
            gcno_content, gcda_content)
        self.assertEqual(html_content, html_generated)


if __name__ == "__main__":
    unittest.main()
