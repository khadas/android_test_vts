#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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

import unittest
import vts.utils.python.common.cmd_utils as cmd_utils
import vts.utils.python.common.cmd_result as cmd_result


class CmdResultTest(unittest.TestCase):
    '''Test methods inside android_device module.'''

    def setUp(self):
        """SetUp tasks"""
        self.res_single_no_error = cmd_result.CmdResult('stdout', '', 0)
        self.res_multiple_no_error = cmd_result.CmdResult('stdout1', '', 0)
        self.res_multiple_no_error.AddResult('stdout2', '', 0)
        self.res_multiple_one_error = cmd_result.CmdResult('stdout1', '', 0)
        self.res_multiple_one_error.AddResult('stdout2', 'stderr2', 1)
        self.res_multiple_one_stderr_only = cmd_result.CmdResult('stdout1', '', 0)
        self.res_multiple_one_stderr_only.AddResult('stdout2', 'stderr2', 0)

    def tearDown(self):
        """TearDown tasks"""

    def test_single_result_data(self):
        """Test the functionally of getting data from single command result."""
        self.assertEqual(self.res_single_no_error.stdout, 'stdout')
        self.assertEqual(self.res_single_no_error.stderr, '')
        self.assertEqual(self.res_single_no_error.returncode, 0)
        self.assertEqual(self.res_single_no_error.stdouts[-1], 'stdout')
        self.assertEqual(self.res_single_no_error.stderrs[-1], '')
        self.assertEqual(self.res_single_no_error.returncodes[-1], 0)

    def test_multiple_result_data(self):
        """Test the functionally of getting data from multiple command result."""
        self.assertEqual(self.res_multiple_no_error.stdout, 'stdout2')
        self.assertEqual(self.res_multiple_no_error.stderr, '')
        self.assertEqual(self.res_multiple_no_error.returncode, 0)
        self.assertEqual(self.res_multiple_no_error.stdouts, ['stdout1', 'stdout2'])
        self.assertEqual(self.res_multiple_no_error.stderrs, ['', ''])
        self.assertEqual(self.res_multiple_no_error.returncodes, [0, 0])

if __name__ == "__main__":
    unittest.main()
