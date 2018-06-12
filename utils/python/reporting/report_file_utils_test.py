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

import datetime
import mock
import os
import unittest

from vts.utils.python.reporting import report_file_utils


def simple_GetGcloudPath():
    """mock function created for _GetGcloudPath"""
    return "gcloud"


def simple_GetGsutilPath():
    """mock function created for _GetGsutilPath"""
    return "gsutil"


def simple_ExecuteOneShellCommand(input_string):
    """mock function created for ExecuteOneShellCommand"""
    std_out = "this is standard output"
    std_err = "this is standard error"
    return_code = 0
    return std_out, std_err, return_code


def simple_PushReportFileGcs(src_path, dest_path):
    """mock function created for _PushReportFileGcs"""
    return


def simple_os_walk(source_dir, followlinks):
    """mock function created for os.walk"""
    root = "/root"
    dirs = ("dir1", "dir2")
    files = ("dir1/file1_1.txt", "dir1/file1_2.txt", "dir1/file1_3.txt",
             "dir2/file2_1")
    return [(root, dirs, files)]


class ReportFileUtilsTest(unittest.TestCase):
    """Unit tests for report_file_utils module"""

    def setUp(self):
        """setup tasks"""
        self.category = "category_default"
        self.name = "name_default"

    def testInitializationDefault(self):
        """Tests the default setting initialization of a UploadGcs object."""
        _report_file_util = report_file_utils.ReportFileUtil()
        self.assertEqual(_report_file_util._flatten_source_dir, False)
        self.assertEqual(_report_file_util._use_destination_date_dir, False)
        self.assertEqual(_report_file_util._source_dir, None)
        self.assertEqual(_report_file_util._destination_dir, None)
        self.assertEqual(_report_file_util._url_prefix, None)

    def testInitializationCustom(self):
        """Tests the custom setting initialization of a UploadGcs object."""
        _report_file_util = report_file_utils.ReportFileUtil(
            flatten_source_dir=True,
            use_destination_date_dir=True,
            destination_dir="gs://vts-log",
            url_prefix="https://storage.cloud.google.com/vts-log/")
        self.assertEqual(_report_file_util._flatten_source_dir, True)
        self.assertEqual(_report_file_util._use_destination_date_dir, True)
        self.assertEqual(_report_file_util._source_dir, None)
        self.assertEqual(_report_file_util._destination_dir, "gs://vts-log")
        self.assertEqual(_report_file_util._url_prefix,
                         "https://storage.cloud.google.com/vts-log/")

    def testConvertReportPath(self):
        """Tests _ConvertReportPath."""
        _report_file_util = report_file_utils.ReportFileUtil(
            flatten_source_dir=False,
            use_destination_date_dir=False,
            destination_dir="gs://vts-log",
            url_prefix="https://storage.cloud.google.com/vts-log/")
        src_path = "dir1/dir1_1/dir_1_1_1/file1"
        root_dir = "dir1/dir1_1"
        file_name_prefix = ""
        dest_path, url = _report_file_util._ConvertReportPath(
            src_path=src_path,
            root_dir=root_dir,
            file_name_prefix=file_name_prefix)
        self.assertEqual(
            dest_path,
            os.path.join(
                _report_file_util._destination_dir,
                os.path.join(
                    os.path.relpath(os.path.dirname(src_path), root_dir),
                    os.path.basename(src_path))))
        self.assertEqual(
            url,
            os.path.join(
                _report_file_util._url_prefix,
                os.path.join(
                    os.path.relpath(os.path.dirname(src_path), root_dir),
                    os.path.basename(src_path))))

    def testConvertReportPathFlatten(self):
        """Tests _ConvertReportPath with flattened source dir."""
        _report_file_util = report_file_utils.ReportFileUtil(
            flatten_source_dir=True,
            use_destination_date_dir=False,
            destination_dir="gs://vts-log",
            url_prefix="https://storage.cloud.google.com/vts-log/")
        src_path = "dir1/dir1_1/dir_1_1_1/file1"
        root_dir = "dir1/dir1_1"
        file_name_prefix = ""
        dest_path, url = _report_file_util._ConvertReportPath(
            src_path=src_path,
            root_dir=root_dir,
            file_name_prefix=file_name_prefix)
        self.assertEqual(
            dest_path,
            os.path.join(_report_file_util._destination_dir,
                         os.path.basename(src_path)))
        self.assertEqual(
            url,
            os.path.join(_report_file_util._url_prefix,
                         os.path.basename(src_path)))

    def testConvertReportPathFlattenDateDir(self):
        """Tests _ConvertReportPath with flattened source dir and date dir."""
        _report_file_util = report_file_utils.ReportFileUtil(
            flatten_source_dir=True,
            use_destination_date_dir=True,
            destination_dir="gs://vts-log",
            url_prefix="https://storage.cloud.google.com/vts-log/")
        src_path = "dir1/dir1_1/dir_1_1_1/file1"
        root_dir = "dir1/dir1_1"
        file_name_prefix = ""
        dest_path, url = _report_file_util._ConvertReportPath(
            src_path=src_path,
            root_dir=root_dir,
            file_name_prefix=file_name_prefix)
        self.assertEqual(
            dest_path,
            os.path.join(
                _report_file_util._destination_dir,
                os.path.join(datetime.datetime.now().strftime('%Y-%m-%d'),
                             os.path.basename(src_path))))
        self.assertEqual(
            url,
            os.path.join(
                _report_file_util._url_prefix,
                os.path.join(datetime.datetime.now().strftime('%Y-%m-%d'),
                             os.path.basename(src_path))))

    @mock.patch(
        'vts.utils.python.gcs.gcs_utils.GcsUtils.GetGsutilPath',
        side_effect=simple_GetGsutilPath)
    @mock.patch(
        'vts.utils.python.common.cmd_utils.ExecuteOneShellCommand',
        side_effect=simple_ExecuteOneShellCommand)
    def testPushReportFileGcs(self, simple_ExecuteOneShellCommand,
                              simple_GetGsutilPath):
        """Tests the _PushReportFileGcs function with mocking"""
        _report_file_util = report_file_utils.ReportFileUtil()
        _report_file_util._PushReportFileGcs("src_path", "dest_path")
        simple_GetGsutilPath.assert_called_with()
        simple_ExecuteOneShellCommand.assert_called_with(
            "gsutil cp src_path dest_path")

    @mock.patch(
        'vts.utils.python.reporting.report_file_utils.ReportFileUtil._PushReportFileGcs',
        side_effect=simple_PushReportFileGcs)
    @mock.patch('os.walk', side_effect=simple_os_walk)
    def testSaveReportsFromDirectory(self, simple_os_walk,
                                     simple_PushReportFileGcs):
        """Tests the SaveReportsFromDirectory function with mocking"""
        _report_file_util = report_file_utils.ReportFileUtil(
            flatten_source_dir=False,
            use_destination_date_dir=False,
            destination_dir="gs://vts-log")
        source_dir = "/root"
        file_name_prefix = "prefix_"
        urls = _report_file_util.SaveReportsFromDirectory(
            source_dir=source_dir,
            file_name_prefix=file_name_prefix,
            file_path_filters=None)
        simple_os_walk.assert_called_with(source_dir, followlinks=False)
        self.assertEqual(simple_PushReportFileGcs.call_count, 4)


if __name__ == "__main__":
    unittest.main()
