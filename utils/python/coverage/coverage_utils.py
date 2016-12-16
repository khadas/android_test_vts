#!/usr/bin/env python
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
import io
import logging
import os
import shutil
import time

from vts.utils.python.archive import archive_parser
from vts.utils.python.coverage import coverage_report
from vts.utils.python.coverage import gcda_parser
from vts.utils.python.coverage import gcno_parser
from vts.utils.python.coverage.parser import FileFormatError

TARGET_COVERAGE_PATH = "/data/local/tmp/"
LOCAL_COVERAGE_PATH = "/tmp/vts-test-coverage"

COVERAGE_SUFFIX = ".gcnodir"
GIT_PROJECT = "git_project"
MODULE_NAME = "module_name"
NAME = "name"
PATH = "path"


def InitializeDeviceCoverage(dut):
    """Initializes the device for coverage before tests run.

    Finds and removes all gcda files under TARGET_COVERAGE_PATH before tests
    run.

    Args:
        dut: the device under test.
    """
    logging.info("Removing existing gcda files.")
    gcda_files = dut.adb.shell("find %s -name \"*.gcda\" -type f -delete" %
                               TARGET_COVERAGE_PATH)


def GetGcdaDict(dut, local_coverage_path=None):
    """Retrieves GCDA files from device and creates a dictionary of files.

    Find all GCDA files on the target device, copy them to the host using
    adb, then return a dictionary mapping from the gcda basename to the
    temp location on the host.

    Args:
        dut: the device under test.
        local_coverage_path: the host path (string) in which to copy gcda files

    Returns:
        A dictionary with gcda basenames as keys and contents as the values.
    """
    logging.info("Creating gcda dictionary")
    gcda_dict = {}
    if not local_coverage_path:
        timestamp = str(int(time.time() * 1000000))
        local_coverage_path = os.path.join(LOCAL_COVERAGE_PATH, timestamp)
    if os.path.exists(local_coverage_path):
        shutil.rmtree(local_coverage_path)
    os.makedirs(local_coverage_path)
    logging.info("Storing gcda tmp files to: %s", local_coverage_path)
    gcda_files = dut.adb.shell("find %s -name \"*.gcda\"" %
                               TARGET_COVERAGE_PATH).split("\n")
    for gcda in gcda_files:
        if gcda:
            basename = os.path.basename(gcda.strip())
            file_name = os.path.join(local_coverage_path,
                                     basename)
            dut.adb.pull("%s %s" % (gcda, file_name))
            gcda_content = open(file_name, "rb").read()
            gcda_dict[basename] = gcda_content
    return gcda_dict

def ProcessCoverageData(report_msg, cov_zip, modules, gcda_dict, revision_dict):
    """Process coverage data and appends coverage reports to the report message.

    Opens the gcno files in the cov_zip for the specified modules and matches
    gcno/gcda files. Then, coverage vectors are generated for each set of matching
    gcno/gcda files and appended as a CoverageReportMessage to the provided
    report message.

    Args:
        report_msg: a TestReportMessage or TestCaseReportMessage object
        cov_zip: the zip file containing gcnodir files from the device build
        modules: the list of module names for which to enable coverage
        gcda_dict: the dictionary of gcda basenames to gcda content (binary string)
        revision_dict: the dictionary with project names as keys and revision ID
                       strings as values.
    """
    covered_modules = set(cov_zip.namelist())
    for module in modules:
        if MODULE_NAME not in module or GIT_PROJECT not in module:
            logging.error("Coverage module must specify name and git project: %s",
                          module)
            continue
        project = module[GIT_PROJECT]
        if PATH not in project or NAME not in project:
            logging.error("Project name and path not specified: %s", project)
            continue

        name = str(module[MODULE_NAME]) + COVERAGE_SUFFIX
        git_project = str(project[NAME])
        git_project_path = str(project[PATH])

        if name not in covered_modules:
            logging.error("No coverage information for module %s", name)
            continue
        if git_project not in revision_dict:
            logging.error("Git project not present in device revision dict: %s",
                          git_project)
            continue

        revision = str(revision_dict[git_project])
        archive = archive_parser.Archive(cov_zip.open(name).read())
        try:
            archive.Parse()
        except ValueError:
            logging.error("Archive could not be parsed: %s" % name)
            continue

        for gcno_file_path in archive.files:
            file_name_path = gcno_file_path.rsplit(".", 1)[0]
            file_name = os.path.basename(file_name_path)
            gcno_content = archive.files[gcno_file_path]
            gcno_stream = io.BytesIO(gcno_content)
            try:
                gcno_summary = gcno_parser.GCNOParser(gcno_stream).Parse()
            except FileFormatError:
                logging.error("Error parsing gcno file %s", gcno_file_path)
                continue
            src_file_path = None

            # Match gcno file with gcda file
            gcda_name = file_name + ".gcda"
            if gcda_name not in gcda_dict:
                logging.error("No gcda file found %s." % gcda_name)
                continue
            gcda_content = gcda_dict[gcda_name]

            # Match gcno file with source files
            for key in gcno_summary.functions:
                src_file_path = gcno_summary.functions[key].src_file_name
                src_parts = src_file_path.rsplit(".", 1)
                src_file_name = src_parts[0]
                src_extension = src_parts[1]
                if src_extension not in ["c", "cpp", "cc"]:
                    logging.warn("Found unsupported file type: %s" %
                                 src_file_path)
                    continue
                if src_file_name.endswith(file_name):
                    logging.info("Coverage source file: %s" %
                                 src_file_path)
                    break

            if not src_file_path:
                logging.error("No source file found for %s." %
                              gcno_file_path)
                continue

            # Process and merge gcno/gcda data
            gcda_stream = io.BytesIO(gcda_content)
            try:
                gcda_parser.GCDAParser(gcda_stream, gcno_summary).Parse()
            except FileFormatError:
                logging.error("Error parsing gcda file %s", gcda_content)
                continue
            coverage_vec = coverage_report.GenerateLineCoverageVector(
                src_file_path, gcno_summary)
            coverage = report_msg.coverage.add()
            coverage.total_line_count, coverage.covered_line_count = (
                coverage_report.GetCoverageStats(coverage_vec))
            coverage.line_coverage_vector.extend(coverage_vec)
            src_file_path = os.path.relpath(src_file_path, git_project_path)
            coverage.file_path = src_file_path
            coverage.revision = revision
            coverage.project_name = git_project
