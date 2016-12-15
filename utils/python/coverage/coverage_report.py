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
to reconstruct a coverage report. GenerateLineCoverageVector() is a helper
function that produces a vector of line counts and GenerateCoverageHTML()
uses the vector and source to produce the HTML coverage report.
"""

import cgi
import io
import logging
import os
from vts.utils.python.coverage import gcda_parser
from vts.utils.python.coverage import gcno_parser

TABLE_STYLE = (
    "border=0 style=\"width: 100%; word-spacing: 5px;"
    "font-family: monospace; white-space:PRE; border-collapse: collapse\"")

COUNT_STYLE = (
    "style=\"white-space:nowrap; text-align:right; border-right: 1px "
    "solid black; padding-right: 5px; font-style: bold\"")

LINE_NO_STYLE = (
    "style=\"padding-left: 35px; white-space:nowrap; padding-right:5px; "
    "border-right: 1px dotted gray\"")

SRC_LINE_STYLE = "style=\"padding-left: 10px;width:99%\""

UNCOVERED_STYLE = "bgcolor=\"LightPink\""

COVERED_STYLE = "bgcolor=\"LightGreen\""


def GenerateLineCoverageVector(src_file_name, src_file_length,
                               gcno_file_content, gcda_file_content):
    """Returns a list of invocation counts for each line in the file.

    Parses the GCNO and GCDA file specified to calculate the number of times
    each line in the source file specified by src_file_name was executed.

    Args:
        src_file_name: string, the source file name.
        src_file_length: int, the number of lines in the src file.
        gcno_file_content: string, the raw gcno binary file content.
        gcda_file_content: string, the raw gcda binary file content.

    Returns:
        A list of integers (or None) representing the number of times the
        i-th line was executed. None indicates a line that is not executable.
    """
    if gcno_file_content:
        logging.info("GenerateLineCoverageVector: gcno_file_content %d bytes",
                     len(gcno_file_content))
    if gcda_file_content:
        logging.info("GenerateLineCoverageVector: gcda_file_content %d bytes",
                     len(gcda_file_content))
    gcno_stream = io.BytesIO(gcno_file_content)
    file_summary = gcno_parser.GCNOParser(gcno_stream).Parse()
    gcda_stream = io.BytesIO(gcda_file_content)
    gcda_parser.GCDAParser(gcda_stream, file_summary).Parse()
    src_lines_counts = [None] * src_file_length
    logging.info("GenerateLineCoverageVector: src file lines %d",
                 src_file_length)
    for ident in file_summary.functions:
        func = file_summary.functions[ident]
        if not src_file_name.endswith(os.path.basename(func.src_file_name)):
            logging.warn("GenerateLineCoverageVector: %s file is skipped",
                         func.src_file_name)
            continue
        for block in func.blocks:
            for line in block.lines:
                logging.info("GenerateLineCoverageVector: covered line %s",
                             line)
                if line >= 0 and line < len(src_lines_counts):
                    if src_lines_counts[line - 1] is None:
                        src_lines_counts[line - 1] = 0
                    src_lines_counts[line - 1] += block.count
                else:
                    logging.error(
                        "GenerateLineCoverageVector: line mismatch %s", line)
    return src_lines_counts


def GenerateCoverageHTML(src_file_content, src_lines_counts):
    """Generates an HTML coverage report given the source and the counts.

    Outputs an HTML file containing coverage information for the provided
    source file. Creates a table with a row for each line of code and a
    cell for the coverage count, line number, and source code.

    Args:
        src_file_content: string, the C/C++ source file content.
        src_lines_counts: A list of integers (or None) representing the
                          number of times the i-th line was executed. None
                          indicates the line is not executable.

    Returns:
       the coverage HTML produced for the src_file_content.
    """
    html = "<div><table %s>\n" % TABLE_STYLE
    src_lines = src_file_content.split('\n')
    for line_no in range(len(src_lines)):
        if src_lines_counts[line_no] is None:  #  Not executable
            html += "<tr>\n<td %s>--</td>\n" % COUNT_STYLE
        elif not src_lines_counts[line_no]:  #  Uncovered line
            html += "<tr %s>\n<td %s>%i</td>\n" % (
                UNCOVERED_STYLE, COUNT_STYLE, src_lines_counts[line_no])
        else:  #  covered line
            html += "<tr %s>\n<td %s>%i</td>\n" % (COVERED_STYLE, COUNT_STYLE,
                                                   src_lines_counts[line_no])
        html += "<td %s>%i</td>\n" % (LINE_NO_STYLE, line_no + 1)
        html += "<td %s>%s</td>\n</tr>\n" % (SRC_LINE_STYLE,
                                             cgi.escape(src_lines[line_no]))
    html += "</table></div>"
    return html


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
    src_lines = src_file_content.split('\n')
    src_lines_counts = GenerateLineCoverageVector(
        src_file_name, len(src_lines), gcno_file_content, gcda_file_content)
    return GenerateCoverageHTML(src_file_content, src_lines_counts)


def GetCoverageStats(src_lines_counts):
    """Returns the coverage stats.

    Args:
        src_lines_counts: A list of integers (or None) representing the number
                          of times the i-th line was executed.
                          None indicates a line that is not executable.

    Returns:
        integer, the number of lines instrumented for coverage measurement
        integer, the number of executed or covered lines
    """
    total = 0
    covered = 0
    if not src_lines_counts or not isinstance(src_lines_counts, list):
        logging.error("GetCoverageStats: input invalid.")
        return total, covered

    for line in src_lines_counts:
        if line is None:
            continue
        total += 1
        if line > 0:
            covered += 1
    return total, covered

