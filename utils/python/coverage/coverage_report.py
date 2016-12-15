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
"""Generates coverage reports using outputs from GCC.

The GenerateCoverageReport() function returns HTML to display the coverage
at each line of code in a provided source file. Coverage information is
parsed from .gcno and .gcda file contents and combined with the source file
to reconstruct a coverage report.
"""

import logging

def GenerateCoverageReport(src_file_name, src_file_content, gcov_file_content,
                           gcda_file_content):
    """Returns the produced html file contents.

    This merges the given GCDA files and produces merged coverage html file for
    each source file.

    Args:
        src_file_name: string, the source file name.
        src_file_content: string, the C/C++ source file content.
        gcov_file_content: string, the raw gcov binary file content.
        gcda_file_content: string, the raw gcda binary file content.

    Returns:
        the coverage HTML produced for 'src_file_name'.
    """
    logging.info("GenerateCoverageReport: src_file_content %s",
                 src_file_content)
    if gcov_file_content:
        logging.info("GenerateCoverageReport: gcov_file_content %d bytes",
                     len(gcov_file_content))
    if gcda_file_content:
        logging.info("GenerateCoverageReport: gcda_file_content %d bytes",
                     len(gcda_file_content))
    return "<table border=0><tr><td>Code coverage will show up at here</table>"
