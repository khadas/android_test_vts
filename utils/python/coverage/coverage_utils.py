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

GCNO_SUFFIX = ".gcno"
GCDA_SUFFIX = ".gcda"
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

def GetChecksumGcnoDict(cov_zip):
    """Generates a dictionary from gcno checksum to GCNOParser object.

    Processes the gcnodir files in the zip file to produce a mapping from gcno
    checksum to the GCNOParser object wrapping the gcno content.

    Args:
        cov_zip: the zip file containing gcnodir files from the device build

    Returns:
        the dictionary of gcno checksums to GCNOParser objects
    """
    checksum_gcno_dict = dict()
    fnames = cov_zip.namelist()
    instrumented_modules = [f for f in fnames if f.endswith(COVERAGE_SUFFIX)]
    for instrumented_module in instrumented_modules:
        # Read the gcnodir file
        archive = archive_parser.Archive(cov_zip.open(instrumented_module).read())
        try:
            archive.Parse()
        except ValueError:
            logging.error("Archive could not be parsed: %s", name)
            continue

        for gcno_file_path in archive.files:
            file_name_path = gcno_file_path.rsplit(".", 1)[0]
            file_name = os.path.basename(file_name_path)
            gcno_stream = io.BytesIO(archive.files[gcno_file_path])
            gcno_file_parser = gcno_parser.GCNOParser(gcno_stream)
            checksum_gcno_dict[gcno_file_parser.checksum] = gcno_file_parser
    return checksum_gcno_dict

def ExtractSourceName(gcno_summary, file_name):
    """Gets the source name from the GCNO summary object.

    Gets the original source file name from the FileSummary object describing
    a gcno file using the base filename of the gcno/gcda file.

    Args:
        gcno_summary: a FileSummary object describing a gcno file
        file_name: the base filename (without extensions) of the gcno or gcda file

    Returns:
        The relative path to the original source file corresponding to the
        provided gcno summary. The path is relative to the root of the build.
    """
    src_file_path = None
    for key in gcno_summary.functions:
        src_file_path = gcno_summary.functions[key].src_file_name
        src_parts = src_file_path.rsplit(".", 1)
        src_file_name = src_parts[0]
        src_extension = src_parts[1]
        if src_extension not in ["c", "cpp", "cc"]:
            logging.warn("Found unsupported file type: %s", src_file_path)
            continue
        if src_file_name.endswith(file_name):
            logging.info("Coverage source file: %s", src_file_path)
            break
    return src_file_path

def AddCoverageReport(report_msg, src_file_path, gcno_summary,
                      git_project_name, git_project_path, revision):
    """Adds a coverage report to the VtsReportMessage.

    Processes the source information, git project information, and processed
    coverage information and stores it into a CoverageReportMessage within the
    report message.

    Args:
        report_msg: a TestReportMessage or TestCaseReportMessage object
        src_file_path: the path to the original source file
        gcno_summary: a FileSummary object describing a gcno file
        git_project_name: the name of the git project containing the source
        git_project_path: the path from the root to the git project
        revision: the commit hash identifying the source code that was used to
                  build a device image
    """
    coverage_vec = coverage_report.GenerateLineCoverageVector(
        src_file_path, gcno_summary)
    coverage = report_msg.coverage.add()
    coverage.total_line_count, coverage.covered_line_count = (
        coverage_report.GetCoverageStats(coverage_vec))
    coverage.line_coverage_vector.extend(coverage_vec)

    src_file_path = os.path.relpath(src_file_path, git_project_path)
    coverage.file_path = src_file_path
    coverage.revision = revision
    coverage.project_name = git_project_name

def AutoProcess(report_msg, checksum_gcno_dict, gcda_dict, revision_dict):
    """Process coverage data and appends coverage reports to the report message.

    Matches gcno files with gcda files and processes them into a coverage report
    with references to the original source code used to build the system image.
    Coverage information is appended as a CoverageReportMessage to the provided
    report message.

    Git project information is automatically extracted from the build info and
    the source file name enclosed in each gcno file. Git project names must
    resemble paths and may differ from the paths to their project root by at
    most one. If no match is found, then coverage information will not be
    be processed.

    e.g. if the project path is test/vts, then its project name may be
         test/vts or <some folder>/test/vts in order to be recognized.

    Args:
        report_msg: a TestReportMessage or TestCaseReportMessage object
        checksum_gcno_dict: the dictionary of gcno checksums to
                            gcno content (binary string)
        gcda_dict: the dictionary of gcda basenames to gcda content (binary string)
        revision_dict: the dictionary with project names as keys and revision ID
                       strings as values.
    """
    for gcda_name in gcda_dict:
        gcda_stream = io.BytesIO(gcda_dict[gcda_name])
        gcda_file_parser = gcda_parser.GCDAParser(gcda_stream)

        if not gcda_file_parser.checksum in checksum_gcno_dict:
            logging.info("No matching gcno file for gcda: %s", gcda_name)
            continue
        gcno_file_parser = checksum_gcno_dict[gcda_file_parser.checksum]

        try:
            gcno_summary = gcno_file_parser.Parse()
        except FileFormatError:
            logging.error("Error parsing gcno file %s", gcno_file_path)
            continue

        file_name = gcda_name.rsplit(".", 1)[0]
        src_file_path = ExtractSourceName(gcno_summary, file_name)

        if not src_file_path:
            logging.error("No source file found for %s.", gcno_file_path)
            continue

        # Process and merge gcno/gcda data
        try:
            gcda_file_parser.Parse(gcno_summary)
        except FileFormatError:
            logging.error("Error parsing gcda file %s", gcda_name)
            continue

        # Get the git project information
        # Assumes that the project name and path to the project root are similar
        revision = None
        for project_name in revision_dict:
            # Matches cases when source file root and project name are the same
            if src_file_path.startswith(str(project_name)):
                git_project_name = str(project_name)
                git_project_path = str(project_name)
                revision = str(revision_dict[project_name])
                logging.info("Source file '%s' matched with project '%s'",
                             src_file_path, git_project_name)
                break

            parts = os.path.normpath(str(project_name)).split(os.sep, 1)
            # Matches when project name has an additional prefix before the
            # project path root.
            if len(parts) > 1 and src_file_path.startswith(parts[-1]):
                git_project_name = str(project_name)
                git_project_path = parts[-1]
                revision = str(revision_dict[project_name])
                logging.info("Source file '%s' matched with project '%s'",
                             src_file_path, git_project_name)

        if not revision:
            logging.info("Could not find git info for %s", src_file_path)
            continue

        AddCoverageReport(report_msg, src_file_path, gcno_summary,
                          git_project_name, git_project_path, revision)


def ManualProcess(report_msg, cov_zip, modules, gcda_dict, revision_dict):
    """Process coverage data and appends coverage reports to the report message.

    Opens the gcno files in the cov_zip for the specified modules and matches
    gcno/gcda files. Then, coverage vectors are generated for each set of matching
    gcno/gcda files and appended as a CoverageReportMessage to the provided
    report message. Unlike AutoProcess, coverage information is only processed
    for the modules explicitly defined in 'modules'.

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
            logging.error("Archive could not be parsed: %s", name)
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
            gcda_name = file_name + GCDA_SUFFIX
            if gcda_name not in gcda_dict:
                logging.error("No gcda file found %s.", gcda_name)
                continue

            src_file_path = ExtractSourceName(gcno_summary, file_name)

            if not src_file_path:
                logging.error("No source file found for %s.", gcno_file_path)
                continue

            # Process and merge gcno/gcda data
            gcda_content = gcda_dict[gcda_name]
            gcda_stream = io.BytesIO(gcda_content)
            try:
                gcda_parser.GCDAParser(gcda_stream).Parse(gcno_summary)
            except FileFormatError:
                logging.error("Error parsing gcda file %s", gcda_content)
                continue

            AddCoverageReport(report_msg, src_file_path, gcno_summary,
                              git_project, git_project_path, revision)


def ProcessCoverageData(report_msg, gcda_dict, revision_dict,
                        checksum_gcno_dict=None, cov_zip=None, modules=None):
    """Process coverage data and appends coverage reports to the report message.

    Calls AutoProcess or ManualProcess depending on the provided inputs. If
    checksum_gcno_dict is provided with a non-None value, then AutoProcess is
    used to generate CoverageReportMessage objects appended to report_msg. If
    both cov_zip and modules are provided with non-None values, then coverage
    information is processed explicitly based on the requested modules using
    ManualProcess.

    Args:
        report_msg: a TestReportMessage or TestCaseReportMessage object
        gcda_dict: the dictionary of gcda basenames to gcda content (binary string)
        revision_dict: the dictionary with project names as keys and revision ID
                       strings as values.
        checksum_gcno_dict: the dictionary of gcno checksums to
                            gcno content (binary string)
        cov_zip: the zip file containing gcnodir files from the device build
        modules: the list of module names for which to enable coverage

    Returns:
        True if the coverage data is processed successfully, False otherwise.
    """
    if checksum_gcno_dict:
        AutoProcess(report_msg, checksum_gcno_dict, gcda_dict, revision_dict)
    elif cov_zip and modules:
        ManualProcess(report_msg, cov_zip, modules, gcda_dict, revision_dict)
    else:
        logging.error("ProcessCoverageData: not enough arguments")
        return False
    return True
