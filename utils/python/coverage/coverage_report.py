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

import cgi
import io
import logging
import os
from vts.utils.python.coverage import gcda_parser
from vts.utils.python.coverage import gcno_parser


def GenerateCoverageReport(src_file_name, src_file_content, gcno_file_content,
                           gcda_file_content):
    """Returns the produced html file contents.

    This produces a coverage html file using the source file as well as the
    GCNO and GCDA file contents.

    Args:
        src_file_name: string, the source file name.
        src_file_content: string, the C/C++ source file content.
        gcno_file_content: string, the raw gcno binary file content.
        gcda_file_content: string, the raw gcda binary file content.

    Returns:
        the coverage HTML produced for 'src_file_name'.
    """
    logging.info("GenerateCoverageReport: src_file_content %s",
                 src_file_content)
    if gcno_file_content:
        logging.info("GenerateCoverageReport: gcno_file_content %d bytes",
                     len(gcno_file_content))
    if gcda_file_content:
        logging.info("GenerateCoverageReport: gcda_file_content %d bytes",
                     len(gcda_file_content))
    gcno_stream = io.BytesIO(gcno_file_content)
    file_summary = gcno_parser.GCNOParser(gcno_stream).Parse()
    gcda_stream = io.BytesIO(gcda_file_content)
    gcda_parser.GCDAParser(gcda_stream, file_summary).Parse()
    src_lines = src_file_content.split('\n')
    src_lines_counts = [None] * len(src_lines)
    logging.info("GenerateCoverageReport: src file lines %d", len(src_lines))
    for ident in file_summary.functions:
        func = file_summary.functions[ident]
        if not src_file_name.endswith(os.path.basename(func.src_file_name)):
            logging.warn("GenerateCoverageReport: %s file is skipped", func.src_file_name)
            continue
        for block in func.blocks:
            for line in block.lines:
                logging.info("GenerateCoverageReport: covered line %s", line)
                if line >= 0 and line < len(src_lines_counts):
                    if src_lines_counts[line - 1] == None:
                        src_lines_counts[line - 1] = 0
                    src_lines_counts[line - 1] += block.count
                else:
                    logging.error("GenerateCoverageReport: line mismatch %s",
                                  line)

    html = (
        "<div><table border=0 style=\"width: 100%; word-spacing: 5px;"
        "font-family: monospace; white-space:PRE; border-collapse: collapse\">\n"
    )
    count_style = (
        "style=\"white-space:nowrap; text-align:right; border-right: 1px "
        "solid black; padding-right: 5px; font-style: bold\"")
    line_no_style = (
        "style=\"padding-left: 35px; white-space:nowrap; padding-right:5px; "
        "border-right: 1px dotted gray\"")
    src_line_style = "style=\"padding-left: 10px;width:99%\""

    for line_no in range(len(src_lines)):
        if src_lines_counts[
                line_no] == None:  #  Not compiled to a machine instruction
            html += "<tr>\n<td %s>--</td>\n" % count_style
        elif not src_lines_counts[line_no]:  #  Uncovered line
            html += "<tr bgcolor=\"LightPink\">\n<td %s>%i</td>\n" % (
                count_style, src_lines_counts[line_no])
        else:  #  covered line
            html += "<tr bgcolor=\"LightGreen\">\n<td %s>%i</td>\n" % (
                count_style, src_lines_counts[line_no])
        html += "<td %s>%i</td>\n" % (line_no_style, line_no)
        html += "<td %s>%s</td>\n</tr>\n" % (src_line_style,
                                         cgi.escape(src_lines[line_no]))
    html += "</table></div>"
    return html
