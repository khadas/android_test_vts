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


class FunctionSummary(object):
    """Summarizes a function and its blocks from .gcno file.

    Attributes:
        blocks: list of BlockSummary objects for each block in the function.
        ident: integer function identifier.
        name: function name.
        src_file_name: name of source file containing the function.
        first_line_number: integer line number at which the function begins in
            srcFile.
    """

    def __init__(self, ident, name, src_file_name, first_line_number):
        """Inits the function summary with provided values.

        Stores the identification string, name, source file name, and
        first line number in the object attributes. Initializes the block
        attribute to the empty list.

        Args:
            ident: integer function identifier.
            name: function name.
            src_file_name: name of source file containing the function.
            first_line_number: integer line number at which the function begins in
              the source file.
        """
        self.blocks = []
        self.ident = ident
        self.name = name
        self.src_file_name = src_file_name
        self.first_line_number = first_line_number

    def ToString(self):
        """Serializes the function summary as a string.

        Returns:
            String representation of the functions and its blocks.
        """
        output = ('Function:  %s : %s\r\n\tFirst Line Number:%i\r\n' %
                  (self.src_file_name, self.name, self.first_line_number))
        for b in self.blocks:
            output += b.ToString()
        return output
