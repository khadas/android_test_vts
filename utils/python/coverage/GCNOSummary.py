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


class GCNOSummary(object):
    """Summarizes .gcno summary file from GCC output.

    Attributes:
    functions: list of FunctionSummary objects for each function described
        in a GCNO file.
    """

    def __init__(self):
        """Inits the object with an empty list in its functions attribute.
        """
        self.functions = []

    def ToString(self):
        """Serializes the summary as a string.

        Returns:
            String representation of the functions, blocks, arcs, and lines.
        """

        output = 'GCNO Summary:\r\n'
        for func in self.functions:
            output += func.ToString()
        return output
