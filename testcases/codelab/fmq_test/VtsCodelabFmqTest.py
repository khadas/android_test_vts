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

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner
from vts.runners.host import const


class VtsCodelabFmqTest(base_test.BaseTestClass):
    """Testcases for API calls on FMQ from host side. """

    def setUpClass(self):
        self.dut = self.android_devices[0]
        # Initialize a hal driver to start all managers on the target side,
        # not used for other purposes.
        self.dut.hal.InitHidlHal(
            target_type="light",
            target_basepaths=self.dut.libPaths,
            target_version_major=2,
            target_version_minor=0,
            target_package="android.hardware.light",
            target_component_name="ILight",
            bits=int(self.abi_bitness))

        # Initialize a non-blocking, synchronized writer.
        self._queue1_writer = self.dut.resource.InitFmq(
            data_type="uint16_t",
            sync=True,
            queue_size=2048,
            blocking=False,
            client=self.dut.hal.GetTcpClient("light"))
        queue1_writer_id = self._queue1_writer.getQueueId()
        asserts.assertNotEqual(queue1_writer_id, -1)

        # Initialize a non-blocking, synchronized reader.
        # This reader shares the same queue as self._queue1_writer.
        self._queue1_reader = self.dut.resource.InitFmq(
            existing_queue=self._queue1_writer,
            client=self.dut.hal.GetTcpClient("light"))
        queue1_reader_id = self._queue1_reader.getQueueId()
        asserts.assertNotEqual(queue1_reader_id, -1)

    def testBasic(self):
        """Tests correctness of basic util methods. """
        asserts.assertEqual(self._queue1_writer.getQuantumSize(), 2)
        asserts.assertEqual(self._queue1_writer.getQuantumCount(), 2048)
        asserts.assertEqual(self._queue1_writer.availableToWrite(), 2048)
        asserts.assertEqual(self._queue1_reader.availableToRead(), 0)
        asserts.assertTrue(self._queue1_writer.isValid(),
                           "Queue writer should be valid.")
        asserts.assertTrue(self._queue1_reader.isValid(),
                           "Queue reader should be valid.")

    def testSimpleReadWrite(self):
        """This test operates on synchronized, non-blocking reader and writer.
           Tests a simple interaction between a writer and a reader.
        """
        write_data = [1, 2, 3, 4, 5]
        read_data = []
        # Writer writes some data.
        asserts.assertTrue(
            self._queue1_writer.write([1, 2, 3, 4, 5], 5),
            "Write queue failed.")
        # Check readers reads them back correctly.
        read_success = self._queue1_reader.read(read_data, 5)
        asserts.assertTrue(read_success, "Read queue failed.")
        asserts.assertEqual(read_data, [1, 2, 3, 4, 5])


if __name__ == "__main__":
    test_runner.main()
