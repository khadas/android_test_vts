#!/usr/bin/env python

# Copyright 2016 Google Inc.
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

import unittest
import logging

from vts.utils.app_engine import bigtable_client
from grpc.framework.interfaces.face.face import AbortionError

class TestMethods(unittest.TestCase):
    """This class defines unit test methods for the vts_gae.Connect.

    Attributes:
        _bigtable_client: An instance of bigtable_client used for performing
            read/write operations.
    """

    _bigtable_client = None

    def setUp(self):
        """This function initiates starting the server in VtsTcpServer."""

        table = 'Test_table'
        self._bigtable_client = bigtable_client.BigTableClient(table)

    def testConnect(self):
        """Unit test for testing successful read/write to bigtable."""

        messages = ['Test Message']
        column_id = 'Test Column'
        try:
            self._bigtable_client.CreateTable()
            self._bigtable_client.Enqueue(messages, column_id)
            self._bigtable_client.Dequeue()
            self._bigtable_client.DeleteTable()
        except AbortionError:
            # Test fails
            logging.exception('AbortionError raised.')

    def testDuplicateTableCreate(self):
        """This function tries to create another table with the same name.

        This should result in error.
        """

        table_name = 'Table_1'
        with self.assertRaises(AbortionError):
            # First delete the existing table
            self._bigtable_client.DeleteTable()
            self._bigtable_client.CreateTable()
            self._bigtable_client.CreateTable()

if __name__ == '__main__':
    unittest.main()
