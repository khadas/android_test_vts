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
import json
import logging
import os
import shutil
import time
import zipfile

from vts.proto import VtsReportMessage_pb2 as ReportMsg
from vts.runners.host import keys
from vts.utils.python.archive import archive_parser
from vts.utils.python.common import cmd_utils
from vts.utils.python.controllers.adb import AdbError
from vts.utils.python.coverage import coverage_report
from vts.utils.python.coverage import gcda_parser
from vts.utils.python.coverage import gcno_parser
from vts.utils.python.coverage.parser import FileFormatError
from vts.utils.python.os import path_utils
from vts.utils.python.web import feature_utils

FLUSH_PATH_VAR = "GCOV_PREFIX"  # environment variable for gcov flush path
TARGET_COVERAGE_PATH = "/data/misc/trace/"  # location to flush coverage
LOCAL_COVERAGE_PATH = "/tmp/vts-test-coverage"  # location to pull coverage to host

# Environment for test process
COVERAGE_TEST_ENV = "GCOV_PREFIX_OVERRIDE=true GCOV_PREFIX=/data/misc/trace/self"

GCNO_SUFFIX = ".gcno"
GCDA_SUFFIX = ".gcda"
COVERAGE_SUFFIX = ".gcnodir"
GIT_PROJECT = "git_project"
MODULE_NAME = "module_name"
NAME = "name"
PATH = "path"
GEN_TAG = "/gen/"

_BUILD_INFO = 'BUILD_INFO'  # name of build info artifact
_GCOV_ZIP = "gcov.zip"  # name of gcov artifact zip
_REPO_DICT = 'repo-dict'  # name of dictionary from project to revision in BUILD_INFO

_FLUSH_COMMAND = (
    "GCOV_PREFIX_OVERRIDE=true GCOV_PREFIX=/data/local/tmp/flusher "
    "/data/local/tmp/vts_coverage_configure flush")
_SP_COVERAGE_PATH = "self"  # relative location where same-process coverage is dumped.

_CHECKSUM_GCNO_DICT = "checksum_gcno_dict"
_COVERAGE_ZIP = "coverage_zip"
_REVISION_DICT = "revision_dict"


class CoverageFeature(feature_utils.Feature):
    """Feature object for coverage functionality.

    Attributes:
        enabled: boolean, True if coverage is enabled, False otherwise
        web: (optional) WebFeature, object storing web feature util for test run
        local_coverage_path: path to store the coverage files.
        _device_resource_dict: a map from device serial number to host resources directory.
        _hal_names: the list of hal names for which to process coverage.
        _coverage_report_file_prefix: prefix of the output coverage report file.
    """

    _TOGGLE_PARAM = keys.ConfigKeys.IKEY_ENABLE_COVERAGE
    _REQUIRED_PARAMS = [keys.ConfigKeys.IKEY_ANDROID_DEVICE]
    _OPTIONAL_PARAMS = [
        keys.ConfigKeys.IKEY_MODULES,
        keys.ConfigKeys.IKEY_OUTPUT_COVERAGE_REPORT,
        keys.ConfigKeys.IKEY_GLOBAL_COVERAGE,
        keys.ConfigKeys.IKEY_EXCLUDE_COVERAGE_PATH
    ]

    def __init__(self, user_params, web=None, local_coverage_path=None):
        """Initializes the coverage feature.

        Args:
            user_params: A dictionary from parameter name (String) to parameter value.
            web: (optional) WebFeature, object storing web feature util for test run
            local_coverage_path: (optional) path to store the .gcda files and coverage reports.
        """
        self.ParseParameters(self._TOGGLE_PARAM, self._REQUIRED_PARAMS,
                             self._OPTIONAL_PARAMS, user_params)
        self.web = web
        self._device_resource_dict = {}
        self._hal_names = None

        if local_coverage_path:
            self.local_coverage_path = local_coverage_path
        else:
            timestamp_seconds = str(int(time.time() * 1000000))
            self.local_coverage_path = os.path.join(LOCAL_COVERAGE_PATH,
                                                    timestamp_seconds)
            if os.path.exists(self.local_coverage_path):
                logging.info("removing existing coverage path: %s",
                             self.local_coverage_path)
                shutil.rmtree(self.local_coverage_path)
            os.makedirs(self.local_coverage_path)
        self._coverage_report_file_prefix = ""

        self.global_coverage = getattr(
            self, keys.ConfigKeys.IKEY_GLOBAL_COVERAGE, True)
        if self.enabled:
            android_devices = getattr(self,
                                      keys.ConfigKeys.IKEY_ANDROID_DEVICE)
            if not isinstance(android_devices, list):
                logging.warn('Android device information not available')
                self.enabled = False
            for device in android_devices:
                serial = device.get(keys.ConfigKeys.IKEY_SERIAL)
                coverage_resource_path = device.get(
                    keys.ConfigKeys.IKEY_GCOV_RESOURCES_PATH)
                if not serial or not coverage_resource_path:
                    logging.warn('Missing coverage information in device: %s',
                                 device)
                    continue
                self._device_resource_dict[str(serial)] = str(coverage_resource_path)

        logging.info("Coverage enabled: %s", self.enabled)

    def _ExtractSourceName(self, gcno_summary, file_name, legacy_build=False, check_name=True):
        """Gets the source name from the GCNO summary object.

        Gets the original source file name from the FileSummary object describing
        a gcno file using the base filename of the gcno/gcda file.

        Args:
            gcno_summary: a FileSummary object describing a gcno file
            file_name: the base filename (without extensions) of the gcno or gcda file
            legacy_build: boolean to indicate whether the file is build with
                          legacy compile system.
            check_name: boolean, whether to compare the source name in gcno_summary with
                        the given file name.

        Returns:
            The relative path to the original source file corresponding to the
            provided gcno summary. The path is relative to the root of the build.
        """
        # Check the source file for the entry function.
        src_file_path = gcno_summary.functions[0].src_file_name
        if not check_name:
            return src_file_path
        src_file_name = src_file_path.rsplit(".", 1)[0]
        # If build with legacy compile system, compare only the base source file
        # name. Otherwise, compare the full source file name (with path info).
        if legacy_build:
            base_src_file_name = os.path.basename(src_file_name)
            return src_file_path if file_name.endswith(
                base_src_file_name) else None
        else:
            return src_file_path if file_name.endswith(src_file_name) else None

    def _GetChecksumGcnoDict(self, cov_zip):
        """Generates a dictionary from gcno checksum to GCNOParser object.

        Processes the gcnodir files in the zip file to produce a mapping from gcno
        checksum to the GCNOParser object wrapping the gcno content.
        Note there might be multiple gcno files corresponds to the same checksum.

        Args:
            cov_zip: the zip file containing gcnodir files from the device build

        Returns:
            the dictionary of gcno checksums to GCNOParser objects
        """
        checksum_gcno_dict = dict()
        fnames = cov_zip.namelist()
        instrumented_modules = [
            f for f in fnames if f.endswith(COVERAGE_SUFFIX)
        ]
        for instrumented_module in instrumented_modules:
            # Read the gcnodir file
            archive = archive_parser.Archive(
                cov_zip.open(instrumented_module).read())
            try:
                archive.Parse()
            except ValueError:
                logging.error("Archive could not be parsed: %s", name)
                continue

            for gcno_file_path in archive.files:
                gcno_stream = io.BytesIO(archive.files[gcno_file_path])
                gcno_file_parser = gcno_parser.GCNOParser(gcno_stream)
                if gcno_file_parser.checksum in checksum_gcno_dict:
                    checksum_gcno_dict[gcno_file_parser.checksum].append(
                        gcno_file_parser)
                else:
                    checksum_gcno_dict[
                        gcno_file_parser.checksum] = [gcno_file_parser]
        return checksum_gcno_dict

    def _ClearTargetGcov(self, dut, path_suffix=None):
        """Removes gcov data from the device.

        Finds and removes all gcda files relative to TARGET_COVERAGE_PATH.
        Args:
            dut: the device under test.
            path_suffix: optional string path suffix.
        """
        path = TARGET_COVERAGE_PATH
        if path_suffix:
            path = path_utils.JoinTargetPath(path, path_suffix)
        try:
            dut.adb.shell("rm -rf %s/*" % TARGET_COVERAGE_PATH)
        except AdbError as e:
            logging.warn('Gcov cleanup error: \"%s\"', e)

    def InitializeDeviceCoverage(self, dut):
        """Initializes the device for coverage before tests run.

        Flushes, then finds and removes all gcda files under
        TARGET_COVERAGE_PATH before tests run.

        Args:
            dut: the device under test.
        """
        try:
            dut.adb.shell(_FLUSH_COMMAND)
        except AdbError as e:
            logging.warn('Command failed: \"%s\"', _FLUSH_COMMAND)
        logging.info("Removing existing gcda files.")
        self._ClearTargetGcov(dut)

    def GetGcdaDict(self, dut):
        """Retrieves GCDA files from device and creates a dictionary of files.

        Find all GCDA files on the target device, copy them to the host using
        adb, then return a dictionary mapping from the gcda basename to the
        temp location on the host.

        Args:
            dut: the device under test.

        Returns:
            A dictionary with gcda basenames as keys and contents as the values.
        """
        logging.info("Creating gcda dictionary")
        gcda_dict = {}
        logging.info("Storing gcda tmp files to: %s", self.local_coverage_path)
        try:
            dut.adb.shell(_FLUSH_COMMAND)
        except AdbError as e:
            logging.warn('Command failed: \"%s\"', _FLUSH_COMMAND)

        gcda_files = set()
        if self._hal_names:
            searchString = "|".join(self._hal_names)
            entries = []
            try:
                entries = dut.adb.shell(
                    'lshal -itp 2> /dev/null | grep -E \"{0}\"'.format(
                        searchString)).splitlines()
            except AdbError as e:
                logging.error('failed to get pid entries')

            pids = set([
                pid.strip()
                for pid in map(lambda entry: entry.split()[-1], entries)
                if pid.isdigit()
            ])
            pids.add(_SP_COVERAGE_PATH)
            for pid in pids:
                path = path_utils.JoinTargetPath(TARGET_COVERAGE_PATH, pid)
                try:
                    files = dut.adb.shell("find %s -name \"*.gcda\"" % path)
                    gcda_files.update(files.split("\n"))
                except AdbError as e:
                    logging.info('No gcda files found in path: \"%s\"', path)

        else:
            try:
                gcda_files.update(
                    dut.adb.shell("find %s -name \"*.gcda\"" %
                                  TARGET_COVERAGE_PATH).split("\n"))
            except AdbError as e:
                logging.warn('No gcda files found in path: \"%s\"',
                            TARGET_COVERAGE_PATH)

        for gcda in gcda_files:
            if gcda:
                basename = os.path.basename(gcda.strip())
                file_name = os.path.join(self.local_coverage_path, basename)
                dut.adb.pull("%s %s" % (gcda, file_name))
                gcda_content = open(file_name, "rb").read()
                gcda_dict[gcda.strip()] = gcda_content
        self._ClearTargetGcov(dut)
        return gcda_dict

    def _OutputCoverageReport(self, isGlobal):
        logging.info("outputing coverage data")
        timestamp_seconds = str(int(time.time() * 1000000))
        coverage_report_file_name = "coverage_report_" + timestamp_seconds + ".txt"
        if self._coverage_report_file_prefix:
            coverage_report_file_name = "coverage_report_" + self._coverage_report_file_prefix + ".txt"

        coverage_report_file = os.path.join(self.local_coverage_path,
                                            coverage_report_file_name)
        logging.info("Storing coverage report to: %s", coverage_report_file)
        coverage_report_msg = ReportMsg.TestReportMessage()
        if isGlobal:
            for c in self.web.report_msg.coverage:
                coverage = coverage_report_msg.coverage.add()
                coverage.CopyFrom(c)
        else:
            for c in self.web.current_test_report_msg.coverage:
                coverage = coverage_report_msg.coverage.add()
                coverage.CopyFrom(c)
        with open(coverage_report_file, 'w+') as f:
            f.write(str(coverage_report_msg))

    def _AutoProcess(self, cov_zip, revision_dict, gcda_dict, isGlobal):
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
            cov_zip: the ZipFile object containing the gcno coverage artifacts.
            revision_dict: the dictionary from project name to project version.
            gcda_dict: the dictionary of gcda basenames to gcda content (binary string)
            isGlobal: boolean, True if the coverage data is for the entire test, False if only for
                      the current test case.
        """
        checksum_gcno_dict = self._GetChecksumGcnoDict(cov_zip)
        output_coverage_report = getattr(
            self, keys.ConfigKeys.IKEY_OUTPUT_COVERAGE_REPORT, False)
        exclude_coverage_path = getattr(
            self, keys.ConfigKeys.IKEY_EXCLUDE_COVERAGE_PATH, None)

        for gcda_name in gcda_dict:
            if GEN_TAG in gcda_name:
                # skip coverage measurement for intermediate code.
                logging.warn("Skip for gcda file: %s", gcda_name)
                continue

            gcda_stream = io.BytesIO(gcda_dict[gcda_name])
            gcda_file_parser = gcda_parser.GCDAParser(gcda_stream)
            file_name = gcda_name.rsplit(".", 1)[0]

            if not gcda_file_parser.checksum in checksum_gcno_dict:
                logging.info("No matching gcno file for gcda: %s", gcda_name)
                continue
            gcno_file_parsers = checksum_gcno_dict[gcda_file_parser.checksum]

            # Find the corresponding gcno summary and source file name for the
            # gcda file.
            src_file_path = None
            gcno_summary = None
            for gcno_file_parser in gcno_file_parsers:
                try:
                    gcno_summary = gcno_file_parser.Parse()
                except FileFormatError:
                    logging.error("Error parsing gcno for gcda %s", gcda_name)
                    break
                legacy_build = "soong/.intermediates" not in gcda_name
                src_file_path = self._ExtractSourceName(
                    gcno_summary, file_name, legacy_build=legacy_build)
                if src_file_path:
                    logging.info("Coverage source file: %s", src_file_path)
                    break

            # If could not find the matched source file name, try to get the
            # name directly form gcno_summary.
            if gcno_summary and not src_file_path:
                src_file_path = self._ExtractSourceName(
                    gcno_summary, file_name, check_name=False)
            if src_file_path:
                logging.info("Coverage source file: %s", src_file_path)
            else:
                logging.error("No source file found for gcda %s.", gcda_name)
                continue

            skip_path = False
            if exclude_coverage_path:
                for path in exclude_coverage_path:
                    base_name = os.path.basename(path)
                    if "." not in base_name:
                        path = path if path.endswith("/") else path + "/"
                    if src_file_path.startswith(path):
                        skip_path = True
                        break

            if skip_path:
                logging.warn("Skip excluded source file %s.", src_file_path)
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

            if self.web and self.web.enabled:
                coverage_vec = coverage_report.GenerateLineCoverageVector(
                    src_file_path, gcno_summary)
                total_count, covered_count = coverage_report.GetCoverageStats(
                    coverage_vec)
                self.web.AddCoverageReport(coverage_vec, src_file_path,
                                           git_project_name, git_project_path,
                                           revision, covered_count,
                                           total_count, isGlobal)

        if output_coverage_report:
            self._OutputCoverageReport(isGlobal)

    # TODO: consider to deprecate the manual process.
    def _ManualProcess(self, cov_zip, revision_dict, gcda_dict, isGlobal):
        """Process coverage data and appends coverage reports to the report message.

        Opens the gcno files in the cov_zip for the specified modules and matches
        gcno/gcda files. Then, coverage vectors are generated for each set of matching
        gcno/gcda files and appended as a CoverageReportMessage to the provided
        report message. Unlike AutoProcess, coverage information is only processed
        for the modules explicitly defined in 'modules'.

        Args:
            cov_zip: the ZipFile object containing the gcno coverage artifacts.
            revision_dict: the dictionary from project name to project version.
            gcda_dict: the dictionary of gcda basenames to gcda content (binary string)
            isGlobal: boolean, True if the coverage data is for the entire test, False if only for
                      the current test case.
        """
        output_coverage_report = getattr(
            self, keys.ConfigKeys.IKEY_OUTPUT_COVERAGE_REPORT, True)
        modules = getattr(self, keys.ConfigKeys.IKEY_MODULES, None)
        covered_modules = set(cov_zip.namelist())
        for module in modules:
            if MODULE_NAME not in module or GIT_PROJECT not in module:
                logging.error(
                    "Coverage module must specify name and git project: %s",
                    module)
                continue
            project = module[GIT_PROJECT]
            if PATH not in project or NAME not in project:
                logging.error("Project name and path not specified: %s",
                              project)
                continue

            name = str(module[MODULE_NAME]) + COVERAGE_SUFFIX
            git_project = str(project[NAME])
            git_project_path = str(project[PATH])

            if name not in covered_modules:
                logging.error("No coverage information for module %s", name)
                continue
            if git_project not in revision_dict:
                logging.error(
                    "Git project not present in device revision dict: %s",
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

                src_file_path = self._ExtractSourceName(gcno_summary,
                                                        file_name)

                if not src_file_path:
                    logging.error("No source file found for %s.",
                                  gcno_file_path)
                    continue

                # Process and merge gcno/gcda data
                gcda_content = gcda_dict[gcda_name]
                gcda_stream = io.BytesIO(gcda_content)
                try:
                    gcda_parser.GCDAParser(gcda_stream).Parse(gcno_summary)
                except FileFormatError:
                    logging.error("Error parsing gcda file %s", gcda_content)
                    continue

                if self.web and self.web.enabled:
                    coverage_vec = coverage_report.GenerateLineCoverageVector(
                        src_file_path, gcno_summary)
                    total_count, covered_count = coverage_report.GetCoverageStats(
                        coverage_vec)
                    self.web.AddCoverageReport(coverage_vec, src_file_path,
                                               git_project, git_project_path,
                                               revision, covered_count,
                                               total_count, isGlobal)

        if output_coverage_report:
            self._OutputCoverageReport(isGlobal)

    def SetCoverageData(self, dut, isGlobal=False):
        """Sets and processes coverage data.

        Organizes coverage data and processes it into a coverage report in the
        current test case

        Requires feature to be enabled; no-op otherwise.

        Args:
            dut:  the device object for which to pull coverage data
            isGlobal: True if the coverage data is for the entire test, False if
                      if the coverage data is just for the current test case.
        """
        if not self.enabled:
            return

        serial = dut.adb.shell('getprop ro.serialno').strip()
        if not serial in self._device_resource_dict:
            logging.error('Invalid device provided: %s', serial)
            return

        gcda_dict = self.GetGcdaDict(dut)
        logging.info("coverage file paths %s", str([fp for fp in gcda_dict]))

        resource_path = self._device_resource_dict[serial]
        if not resource_path:
            logging.error('coverage resource path not found.')
            return

        cov_zip = zipfile.ZipFile(os.path.join(resource_path, _GCOV_ZIP))

        revision_dict = json.load(
            open(os.path.join(resource_path, _BUILD_INFO)))[_REPO_DICT]

        if not hasattr(self, keys.ConfigKeys.IKEY_MODULES):
            # auto-process coverage data
            self._AutoProcess(cov_zip, revision_dict, gcda_dict, isGlobal)
        else:
            # explicitly process coverage data for the specified modules
            self._ManualProcess(cov_zip, revision_dict, gcda_dict, isGlobal)

        # cleanup the downloaded gcda files.
        results = cmd_utils.ExecuteShellCommand(
            "rm -rf %s" % path_utils.JoinTargetPath(self.local_coverage_path,
                                                    "*.gcda"))
        if any(results[cmd_utils.EXIT_CODE]):
            logging.error("Fail to cleanup gcda files.")

    def SetHalNames(self, names=[]):
        """Sets the HAL names for which to process coverage.

        Args:
            names: list of strings, names of hal (e.g. android.hardware.light@2.0)
        """
        self._hal_names = list(names)

    def SetCoverageReportFilePrefix(self, prefix):
        """Sets the prefix for outputting the coverage report file.

        Args:
            prefix: strings, prefix of the coverage report file.
        """
        self._coverage_report_file_prefix = prefix
