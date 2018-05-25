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

from vts.utils.python.instrumentation import test_framework_instrumentation as tfi
from vts.utils.python.instrumentation import test_framework_instrumentation_event as tfie
from vts.utils.python.instrumentation import test_framework_instrumentation_test_submodule as tfits

class TestFrameworkInstrumentationTest(unittest.TestCase):
    """Unit tests for test_framework_instrumentation module"""

    def setUp(self):
        """Setup tasks"""
        self.category = 'category_default'
        self.name = 'name_default'

    def testEventName(self):
        """Tests whether illegal characters are being recognized and replaced."""
        for name in tfie.ILLEGAL_CHARS:
            event = tfie.TestFrameworkInstrumentationEvent(name, '')
            self.assertNotEqual(event.category, name, 'name %s should not be accepted.' % name)

    def testEventMatch(self):
        """Tests whether Event object can match with a category and name."""
        category = '1'
        name = '2'
        event = tfie.TestFrameworkInstrumentationEvent(category, name)
        self.assertTrue(event.Match(category, name))
        self.assertFalse(event.Match(category, '3'))

    def testEndAlreadyEnded(self):
        """Tests End command on already ended event."""
        event = tfi.Begin(self.category, self.name)
        tfi.End(event)
        self.assertIsNone(event.error)
        tfi.End(event)
        self.assertTrue(event.error)

    def testEndMatch(self):
        """Tests End command with name matching."""
        event = tfi.Begin(self.category, self.name)
        self.assertEqual(event.status, 1)
        tfi.End(self.category, self.name)
        self.assertEqual(event.status, 2)
        self.assertIsNone(event.error)

    def testEndFromOtherModule(self):
        """Tests the use of End command from another module."""
        event = tfi.Begin(self.category, self.name)
        self.assertEqual(event.status, 1)
        tfits.TestFrameworkInstrumentationTestSubmodule().End(self.category, self.name)
        self.assertEqual(event.status, 2)
        self.assertIsNone(event.error)


if __name__ == "__main__":
    unittest.main()