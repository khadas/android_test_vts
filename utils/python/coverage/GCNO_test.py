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

import unittest
import math
import os
import struct

from vts.utils.python.coverage import GCNO
from vts.utils.python.coverage import FunctionSummary
from vts.utils.python.coverage import BlockSummary
from vts.utils.python.coverage import ArcSummary

class MockStream(object):
    """MockStream object allows for mocking file reading behavior.

    Allows for adding integers and strings to the file stream in a
    specified byte format and then reads them as if from a file.

    Attributes:
        content: the byte list representing a file stream
        cursor: the index into the content such that everything before it
                has been read already.
    """

    def __init__(self, format='<'):
        self.content = struct.pack(format + 'III', GCNO.Parser.NOTE_MAGIC, 0,
                                   0)
        self.cursor = 0
        self.format = format

    def read(self, n_bytes):
        """Reads the specified number of bytes from the content stream.

        Args:
            n_bytes: integer number of bytes to read.

        Returns:
            The string of length n_bytes beginning at the cursor location
            in the content stream.
        """
        content = self.content[self.cursor:self.cursor + n_bytes]
        self.cursor += n_bytes
        return content

    def concat_int(self, integer):
        """Concatenates an integer in binary format to the content stream.

        Args:
            integer: the integer to be concatenated to the content stream.
        """
        self.content += struct.pack(self.format + 'I', integer)

    def concat_string(self, string):
        """Concatenates a string in binary format to the content stream.

        Preceeds the string with an integer representing the number of
        words in the string. Pads the string so that it is word-aligned.

        Args:
            string: the string to be concatenated to the content stream.
        """
        byte_count = len(string)
        word_count = int(math.ceil(byte_count / 4.0))
        padding = '\x00' * (4 * word_count - byte_count)
        self.content += struct.pack('<I', word_count) + bytes(string + padding)


class GCNOTest(unittest.TestCase):
    """Tests for GCNO parser of vts.utils.python.coverage.

    Ensures error handling, byte order detection, and correct
    parsing of functions, blocks, arcs, and lines.
    """
    def setUp(self):
      """Creates a stream for each test.
      """
      self.stream = MockStream()

    def testLittleEndiannessInitialization(self):
        """Tests parser init  with little-endian byte order.

        Verifies that the byte-order is correctly detected.
        """
        parser = GCNO.Parser(self.stream)
        self.assertEqual(parser.format, '<')

    def testBigEndiannessInitialization(self):
        """Tests parser init with big-endian byte order.

        Verifies that the byte-order is correctly detected.
        """
        self.stream = MockStream(format='>')
        parser = GCNO.Parser(self.stream)
        self.assertEqual(parser.format, '>')

    def testBadInitialization(self):
        """Tests error behavior when flag is invalid.
        """
        self.stream.content = struct.pack('<III', 0, 0, 0)
        self.assertRaises(GCNO.FileFormatError, GCNO.Parser, self.stream)

    def testReadIntNormal(self):
        """Asserts that integers are correctly read from the stream.

        Tests the normal case--when the value is actually an integer.
        """
        integer = 2016
        self.stream.concat_int(integer)
        parser = GCNO.Parser(self.stream)
        self.assertEqual(parser.ReadInt(), integer)

    def testReadIntEof(self):
        """Asserts that an error is thrown when the EOF is reached.
        """
        parser = GCNO.Parser(self.stream)
        self.assertRaises(GCNO.FileFormatError, parser.ReadInt)

    def testReadStringNormal(self):
        """Asserts that strings are correctly read from the stream.

        Tests the normal case--when the string is correctly formatted.
        """
        test_string = "This is a test."
        self.stream.concat_string(test_string)
        parser = GCNO.Parser(self.stream)
        self.assertEqual(parser.ReadString(), test_string)

    def testReadStringError(self):
        """Asserts that invalid string format raises error.

        Tests when the string length is too short and EOF is reached.
        """
        test_string = "This is a test."
        byte_count = len(test_string)
        word_count = int(round(byte_count / 4.0))
        padding = '\x00' * (4 * word_count - byte_count)
        test_string_padded = test_string + padding
        content = struct.pack('<I', word_count + 1)  #  will cause EOF error
        content += bytes(test_string_padded)
        self.stream.content += content
        parser = GCNO.Parser(self.stream)
        self.assertRaises(GCNO.FileFormatError, parser.ReadString)

    def testReadFunction(self):
        """Asserts that the function is read correctly.

        Verifies that ident, name, source file name,
        and first line number are all read correctly.
        """
        ident = 102010
        self.stream.concat_int(ident)
        self.stream.concat_int(0)
        self.stream.concat_int(0)
        name = "TestFunction"
        src_file_name = "TestSouceFile.c"
        first_line_number = 102
        self.stream.concat_string(name)
        self.stream.concat_string(src_file_name)
        self.stream.concat_int(first_line_number)
        parser = GCNO.Parser(self.stream)
        summary = parser.ReadFunction()
        self.assertEqual(name, summary.name)
        self.assertEqual(ident, summary.ident)
        self.assertEqual(src_file_name, summary.src_file_name)
        self.assertEqual(first_line_number, summary.first_line_number)

    def testReadBlocks(self):
        """Asserts that blocks are correctly read from the stream.

        Tests correct values for flag and index.
        """
        n_blocks = 10
        func = FunctionSummary.FunctionSummary(0, "func", "src.c", 1)
        for i in range(n_blocks):
            self.stream.concat_int(3 * i)
        parser = GCNO.Parser(self.stream)
        parser.ReadBlocks(n_blocks, func)
        self.assertEqual(len(func.blocks), n_blocks)
        for i in range(n_blocks):
            self.assertEqual(func.blocks[i].flag, 3 * i)
            self.assertEqual(func.blocks[i].index, i)

    def testReadArcsNormal(self):
        """Asserts that arcs are correctly read from the stream.

        Does not test the use of flags. Validates that arcs are
        created in both blocks and the source/destination are
        correct for each.
        """
        n_blocks = 50
        func = FunctionSummary.FunctionSummary(0, "func", "src.c", 1)
        func.blocks = [BlockSummary.BlockSummary(i, 3 * i)
                       for i in range(n_blocks)]
        src_block_index = 0
        skip = 2
        self.stream.concat_int(src_block_index)
        for i in range(src_block_index + 1, n_blocks, skip):
            self.stream.concat_int(i)
            self.stream.concat_int(0)  #  no flag applied to the arc
        parser = GCNO.Parser(self.stream)
        n_arcs = len(range(src_block_index + 1, n_blocks, skip))
        parser.ReadArcs(n_arcs * 2 + 1, func)
        j = 0
        for i in range(src_block_index + 1, n_blocks, skip):
            self.assertEqual(
                func.blocks[src_block_index].exit_arcs[j].src_block_index,
                src_block_index)
            self.assertEqual(
                func.blocks[src_block_index].exit_arcs[j].dst_block_index, i)
            self.assertEqual(func.blocks[i].entry_arcs[0].src_block_index,
                             src_block_index)
            self.assertEqual(func.blocks[i].entry_arcs[0].dst_block_index, i)
            j += 1

    def testReadArcFlags(self):
        """Asserts that arc flags are correctly interpreted.
        """
        n_blocks = 5
        func = FunctionSummary.FunctionSummary(0, "func", "src.c", 1)
        func.blocks = [BlockSummary.BlockSummary(i, 3 * i)
                       for i in range(n_blocks)]
        self.stream.concat_int(0)  #  source block index

        self.stream.concat_int(1)  #  normal arc
        self.stream.concat_int(0)

        self.stream.concat_int(2)  #  on-tree arc
        self.stream.concat_int(ArcSummary.ArcSummary.GCOV_ARC_ON_TREE)

        self.stream.concat_int(3)  #  fake arc
        self.stream.concat_int(ArcSummary.ArcSummary.GCOV_ARC_FAKE)

        self.stream.concat_int(4)  #  fallthrough arc
        self.stream.concat_int(ArcSummary.ArcSummary.GCOV_ARC_FALLTHROUGH)

        parser = GCNO.Parser(self.stream)
        parser.ReadArcs(4 * 2 + 1, func)

        self.assertFalse(func.blocks[0].exit_arcs[0].on_tree)
        self.assertFalse(func.blocks[0].exit_arcs[0].fake)
        self.assertFalse(func.blocks[0].exit_arcs[0].fallthrough)

        self.assertTrue(func.blocks[0].exit_arcs[1].on_tree)
        self.assertFalse(func.blocks[0].exit_arcs[1].fake)
        self.assertFalse(func.blocks[0].exit_arcs[1].fallthrough)

        self.assertFalse(func.blocks[0].exit_arcs[2].on_tree)
        self.assertTrue(func.blocks[0].exit_arcs[2].fake)
        self.assertFalse(func.blocks[0].exit_arcs[2].fallthrough)

        self.assertFalse(func.blocks[0].exit_arcs[3].on_tree)
        self.assertFalse(func.blocks[0].exit_arcs[3].fake)
        self.assertTrue(func.blocks[0].exit_arcs[3].fallthrough)

    def testReadLines(self):
        """Asserts that lines are read correctly.

        Blocks must have correct references to the lines contained
        in the block.
        """
        self.stream.concat_int(2)  #  block number
        self.stream.concat_int(0)  #  dummy
        name = "src.c"
        self.stream.concat_string(name)
        for i in range(1, 6):
            self.stream.concat_int(i)

        n_blocks = 5
        func = FunctionSummary.FunctionSummary(0, "func", name, 1)
        func.blocks = [BlockSummary.BlockSummary(i, 3 * i)
                       for i in range(n_blocks)]
        parser = GCNO.Parser(self.stream)
        parser.ReadLines(5 + len(name), func)
        self.assertEqual(len(func.blocks[2].lines), 5)
        self.assertEqual(func.blocks[2].lines, range(1, 6))

    def testSampleFile(self):
        """Asserts correct parsing of sample GCNO file.

        Verifies the blocks and lines for each function in
        the file.
        """
        path = os.path.join(
            os.getenv('ANDROID_BUILD_TOP'),
            'test/vts/utils/python/coverage/testdata/sample.gcno')
        summary = GCNO.parse(path)
        self.assertEqual(len(summary.functions), 2)

        # Check function: testFunctionName
        func = summary.functions[0]
        self.assertEqual(func.name, 'testFunctionName')
        self.assertEqual(func.src_file_name, 'sample.c')
        self.assertEqual(func.first_line_number, 35)
        self.assertEqual(len(func.blocks), 5)
        expected_list = [[], [], [35, 40, 41], [42], []]
        for index, expected in zip(range(5), expected_list):
            self.assertEqual(func.blocks[index].lines, expected)

        # Check function: main
        func = summary.functions[1]
        self.assertEqual(func.name, 'main')
        self.assertEqual(func.first_line_number, 5)
        self.assertEqual(len(func.blocks), 12)
        self.assertEqual(func.blocks[0].lines, [])
        expected_list = [[], [],
                         [5 ,11 ,12 ,13],
                         [15], [17], [18], [20],
                         [23, 24, 25],
                         [26, 25],
                         [], [29], [31]]
        for index,expected in zip(range(12), expected_list):
            self.assertEqual(func.blocks[index].lines, expected)

if __name__ == "__main__":
    unittest.main()
