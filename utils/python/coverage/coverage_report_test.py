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
    """Unit tests for CoverageReport of vts.utils.python.coverage.
    """

    @classmethod
    def setUpClass(cls):
        root_dir = os.path.join(
            os.getenv('ANDROID_BUILD_TOP'),
            'test/vts/utils/python/coverage/testdata/')
        src_path = os.path.join(root_dir, 'sample.c')
        with open(src_path, 'r') as file:
            cls.src_content = file.read()
        gcno_path = os.path.join(root_dir, 'sample.gcno')
        with open(gcno_path, 'rb') as file:
            cls.gcno_content = file.read()
        gcda_path = os.path.join(root_dir, 'sample.gcda')
        with open(gcda_path, 'rb') as file:
            cls.gcda_content = file.read()
        html_path = os.path.join(root_dir, 'sample_coverage.html')
        with open(html_path, 'r') as file:
            cls.html_content = file.read()

    def testGenerateCoverageHTML(self):
        """Tests that coverage HTML is correctly generated.

        Runs GenerateCoverageHTML on mocked source file and count
        files.
        """
        src = 'test\nfile\ncontents'
        src_lines = src.split('\n')
        src_lines_counts = [10, None, 0]
        expected = []
        expected.append('<div><table %s>' % coverage_report.TABLE_STYLE)
        expected.append('<tr %s>' % coverage_report.COVERED_STYLE)
        expected.append('<td %s>%i</td>' %
                     (coverage_report.COUNT_STYLE, src_lines_counts[0]))
        expected.append('<td %s>%i</td>' % (coverage_report.LINE_NO_STYLE, 1))
        expected.append('<td %s>%s</td>' %
                        (coverage_report.SRC_LINE_STYLE, src_lines[0]))
        expected.append('</tr>')

        expected.append('<tr>')
        expected.append('<td %s>%s</td>' % (coverage_report.COUNT_STYLE, '--'))
        expected.append('<td %s>%i</td>' % (coverage_report.LINE_NO_STYLE, 2))
        expected.append('<td %s>%s</td>' %
                        (coverage_report.SRC_LINE_STYLE, src_lines[1]))
        expected.append('</tr>')

        expected.append('<tr %s>' % coverage_report.UNCOVERED_STYLE)
        expected.append('<td %s>%i</td>' %
                        (coverage_report.COUNT_STYLE, src_lines_counts[2]))
        expected.append('<td %s>%i</td>' % (coverage_report.LINE_NO_STYLE, 3))
        expected.append('<td %s>%s</td>' %
                        (coverage_report.SRC_LINE_STYLE, src_lines[2]))
        expected.append('</tr>')
        expected.append('</table></div>')

        generated = coverage_report.GenerateCoverageHTML(src, src_lines_counts)
        self.assertEqual(generated, '\n'.join(expected))

    def testGenerateLineCoverageVector(self):
        """Tests that coverage vector is correctly generated.

        Runs GenerateLineCoverageVector on sample file and checks
        result.
        """
        src_lines = self.src_content.split('\n')
        src_lines_counts = coverage_report.GenerateLineCoverageVector(
            'sample.c', len(src_lines), self.gcno_content, self.gcda_content)
        expected = [None, None, None, None, 2, None, None, None, None, None, 2,
                    2, 2, None, 2, None, 2, 0, None, 2, None, None, 2, 2, 502,
                    500, None, None, 2, None, 2, None, None, None, 2, None,
                    None, None, None, 2, 2, 2, None]
        self.assertEqual(src_lines_counts, expected)

    def testSampleFileReportEndToEnd(self):
        """Verifies that sample file coverage is generated correctly (end-to-end)
        """

        html_generated = coverage_report.GenerateCoverageReport(
            'sample.c', self.src_content, self.gcno_content, self.gcda_content)
        self.assertEqual(self.html_content, html_generated)


if __name__ == "__main__":
    unittest.main()
