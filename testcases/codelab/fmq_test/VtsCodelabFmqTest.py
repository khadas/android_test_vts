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
import random
import time

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

        # Initialize a blocking, synchronized writer.
        self._queue2_writer = self.dut.resource.InitFmq(
            data_type="uint32_t",
            sync=True,
            queue_size=2048,
            blocking=True,
            client=self.dut.hal.GetTcpClient("light"))
        queue2_writer_id = self._queue2_writer.getQueueId()
        asserts.assertNotEqual(queue2_writer_id, -1)

        # Initialize a blocking, synchronized reader.
        # This reader shares the same queue as self._queue2_writer.
        self._queue2_reader = self.dut.resource.InitFmq(
            existing_queue=self._queue2_writer,
            client=self.dut.hal.GetTcpClient("light"))
        queue2_reader_id = self._queue2_reader.getQueueId()
        asserts.assertNotEqual(queue2_reader_id, -1)

        # Initialize a non-blocking, unsynchronized writer.
        self._queue3_writer = self.dut.resource.InitFmq(
            data_type="double_t",
            sync=False,
            queue_size=2048,
            blocking=False,
            client=self.dut.hal.GetTcpClient("light"))
        queue3_writer_id = self._queue3_writer.getQueueId()
        asserts.assertNotEqual(queue3_writer_id, -1)

        # Initialize a non-blocking, unsynchronized reader 1.
        # This reader shares the same queue as self._queue3_writer.
        self._queue3_reader1 = self.dut.resource.InitFmq(
            existing_queue=self._queue3_writer,
            client=self.dut.hal.GetTcpClient("light"))
        queue3_reader1_id = self._queue3_reader1.getQueueId()
        asserts.assertNotEqual(queue3_reader1_id, -1)

        # Initialize a non-blocking, unsynchronized reader 2.
        # This reader shares the same queue as self._queue3_writer and self._queue3_reader1.
        self._queue3_reader2 = self.dut.resource.InitFmq(
            existing_queue=self._queue3_writer,
            client=self.dut.hal.GetTcpClient("light"))
        queue3_reader2_id = self._queue3_reader2.getQueueId()
        asserts.assertNotEqual(queue3_reader2_id, -1)

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
        write_data = generateRandomIntegers(2048)
        read_data = []
        # Writer writes some data.
        asserts.assertTrue(
            self._queue1_writer.write(write_data, 2048), "Write queue failed.")
        # Check readers reads them back correctly.
        read_success = self._queue1_reader.read(read_data, 2048)
        asserts.assertTrue(read_success, "Read queue failed.")
        asserts.assertEqual(read_data, write_data)

    def testReadEmpty(self):
        """Tests reading from an empty queue. """
        read_data = []
        read_success = self._queue1_reader.read(read_data, 5)
        asserts.assertFalse(read_success,
                            "Read should fail because queue is empty.")

    def testWriteFull(self):
        """Tests writes fail when queue is full. """
        write_data = generateRandomIntegers(2048)
        # This write should succeed.
        asserts.assertTrue(
            self._queue1_writer.write(write_data, 2048),
            "Writer should write successfully.")
        # This write should fail because queue is full.
        asserts.assertFalse(
            self._queue1_writer.write(write_data, 2048),
            "Writer should fail because queue is full.")

    def testWriteTooLarge(self):
        """Tests writing more than queue capacity. """
        write_data = generateRandomIntegers(2049)
        asserts.assertFalse(
            self._queue1_writer.write(write_data, 2049),
            "Writer should fail since there are too many items to write.")

    def testConsecutiveReadWrite(self):
        """This test operates on synchronized, nonblocking reader and writer.
           Tests consecutive interaction between reader and writer.
        """
        for i in range(64):
            write_data = generateRandomIntegers(2048)
            asserts.assertTrue(
                self._queue1_writer.write(write_data, 2048),
                "Writer should write successfully.")
            read_data = []
            read_success = self._queue1_reader.read(read_data, 2048)
            asserts.assertTrue(read_success,
                               "Reader should read successfully.")
            asserts.assertEqual(write_data, read_data)

        # Reader should have no more available to read.
        asserts.assertEqual(0, self._queue1_reader.availableToRead())

    def testBlockingReadWrite(self):
        """This test operates on synchronized, blocking reader and writer.
           Tests blocking read/write.
           Writer waits 0.05s and writes.
           Reader blocks for at most 0.1s, and should read successfully.
           TODO: support this operation when reader and writer operates
                 in parallel.
        """
        write_data = generateRandomIntegers(2048)
        read_data = []

        # Writer waits for 0.05s and writes.
        time.sleep(0.05)
        asserts.assertTrue(
            self._queue2_writer.writeBlocking(write_data, 2048, 1000000),
            "Writer should write successfully.")

        # Reader reads.
        read_success = self._queue2_reader.readBlocking(
            read_data, 2048, 1000 * 1000000)
        asserts.assertTrue(read_success,
                           "Reader should read successfully after blocking.")
        asserts.assertEqual(write_data, read_data)

    def testBlockingTimeout(self):
        """This test operates on synchronized, blocking reader.
           Reader should time out because there is not data available.
        """
        read_data = []
        read_success = self._queue2_reader.readBlocking(
            read_data, 5, 100 * 1000000)
        asserts.assertFalse(
            read_success,
            "Reader blocking should time out because there is no data to read."
        )

    def testUnsynchronizedReadWrite(self):
        """This test operates on unsynchronized, nonblocking reader and writer.
           Two readers can read back what writer writes.
        """
        write_data = generateRandomFloats(2048)
        read_data1 = []
        read_data2 = []
        asserts.assertTrue(
            self._queue3_writer.write(write_data, 2048),
            "Writer should write successfully.")
        read_success1 = self._queue3_reader1.read(read_data1, 2048)
        read_success2 = self._queue3_reader2.read(read_data2, 2048)
        asserts.assertTrue(read_success1, "Reader 1 should read successfully.")
        asserts.assertTrue(read_success2, "Reader 2 should read successfully.")
        asserts.assertEqual(write_data, read_data1)
        asserts.assertEqual(write_data, read_data2)

    def testIllegalBlocking(self):
        """This test operates on unsynchronized, nonblocking writer.
           Blocking is not allowed in unsynchronized queue.
        """
        write_data = generateRandomFloats(2048)
        asserts.assertFalse(
            self._queue3_writer.writeBlocking(write_data, 2048, 100 * 1000000),
            "Blocking operation should fail in unsynchronized queue.")


def generateRandomIntegers(data_size):
    """Helper method to generate a list of  random integers between 0 and 100.

    Args:
        data_size: int, length of result list.

    Returns:
        int list, list of integers.
    """
    return [random.randint(0, 100) for i in range(data_size)]


def generateRandomFloats(data_size):
    """Helper method to generate a list of random floats between 0.0 and 100.0.

    Args:
        data_size: int, length of result list.

    Returns:
        float list, list of floats.
    """
    return [random.random() * 100 for i in range(data_size)]

if __name__ == "__main__":
    test_runner.main()
