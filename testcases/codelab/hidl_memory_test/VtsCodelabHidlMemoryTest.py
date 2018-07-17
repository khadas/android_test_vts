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

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner


class VtsCodelabHidlMemoryTest(base_test.BaseTestClass):
    """Tests hidl_memory API from host side.

    Attributes:
        _mem_obj: memory mirror object that users can use to read from or
                  write into.
    """

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

        self._mem_obj = self.dut.resource.InitHidlMemory(
            mem_size=100, client=self.dut.hal.GetTcpClient("light"))

    def testGetSize(self):
        """Tests getting memory size correctness. """
        asserts.assertEqual(100, self._mem_obj.getSize())

    def testSimpleWriteRead(self):
        """Tests writing to the memory and reading the same data back. """
        write_data = "abcdef"
        # Write data into memory.
        self._mem_obj.update()
        self._mem_obj.updateBytes(write_data, len(write_data))
        self._mem_obj.commit()

        # Read data from memory.
        self._mem_obj.read()
        read_data = self._mem_obj.readBytes(len(write_data))
        asserts.assertEqual(write_data, read_data)

    def testLargeWriteRead(self):
        """Tests consecutive writes and reads using integers.

        For each of the 5 iterations, write 5 integers into
        different chunks of memory, and read them back.
        """
        for i in range(5):
            # Writes five integers.
            write_data = [random.randint(0, 100) for j in range(10)]
            write_data_str = str(bytearray(write_data))
            # Start writing at offset i * 5.
            self._mem_obj.updateRange(i * 5, len(write_data_str))
            self._mem_obj.updateBytes(write_data_str, len(write_data_str),
                                      i * 5)
            self._mem_obj.commit()

            # Reads data back.
            self._mem_obj.readRange(i * 5, len(write_data_str))
            read_data_str = self._mem_obj.readBytes(len(write_data_str), i * 5)
            read_data = list(bytearray(read_data_str))
            # Check if read data is correct.
            asserts.assertEqual(write_data, read_data)

    def testWriteTwoRegionsInOneBuffer(self):
        """Tests writing into different regions in one memory buffer.

        Writer requests the beginning of the first half and
        the beginning of the second half of the buffer.
        It writes to the second half, commits, and reads the data back.
        Then it writes to the first half, commits, and reads the data back.
        """
        write_data1 = "abcdef"
        write_data2 = "ghijklmno"

        # Reserve both regions.
        self._mem_obj.updateRange(0, len(write_data1))
        self._mem_obj.updateRange(50, len(write_data2))
        # Write to the second region.
        self._mem_obj.updateBytes(write_data2, len(write_data2), 50)
        self._mem_obj.commit()

        # Read from the second region.
        self._mem_obj.read()
        read_data2 = self._mem_obj.readBytes(len(write_data2), 50)
        self._mem_obj.commit()
        # Check if read data is correct.
        asserts.assertEqual(read_data2, write_data2)

        # Write to the first region.
        self._mem_obj.updateBytes(write_data1, len(write_data1))
        self._mem_obj.commit()

        # Read from the first region.
        self._mem_obj.read()
        read_data1 = self._mem_obj.readBytes(len(write_data1))
        self._mem_obj.commit()
        # Check if read data is correct.
        asserts.assertEqual(write_data1, read_data1)


if __name__ == "__main__":
    test_runner.main()
