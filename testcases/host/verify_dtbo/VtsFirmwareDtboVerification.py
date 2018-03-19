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
#
"""VTS tests to verify DTBO partition/DT overlay application."""

import logging
import os
import shutil
import subprocess
import tempfile

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.android import api
from vts.utils.python.file import target_file_utils
from vts.utils.python.os import path_utils

DTBO_PARTITION_PATH = "/dev/block/bootdevice/by-name/dtbo"  # path to DTBO partition.
DEVICE_TEMP_DIR = "/data/local/tmp/"  # temporary dir in device.
FDT_PATH = "/sys/firmware/fdt"  # path to device tree.
PROPERTY_SLOT_SUFFIX = "ro.boot.slot_suffix"  # indicates current slot suffix for A/B devices


class VtsFirmwareDtboVerification(base_test.BaseTestClass):
    """Validates DTBO partition and DT overlay application.

    Attributes:
        temp_dir: The temporary directory on host.
    """

    def setUpClass(self):
        """Initializes the DUT and creates temporary directories."""
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell
        self.adb = self.dut.adb
        self.temp_dir = tempfile.mkdtemp()
        logging.info("Create %s", self.temp_dir)
        device_path = str(
            path_utils.JoinTargetPath(DEVICE_TEMP_DIR, self.abi_bitness))
        self.shell.Execute("mkdir %s -p" % device_path)

    def setUp(self):
        """Checks if the the preconditions to run the test are met."""
        asserts.skipIf("x86" in self.dut.cpu_abi, "Skipping test for x86 ABI")
        dtbo_exists, _ = self.dtboPartitionPresent()
        if dtbo_exists:
            return
        try:
            api_level = int(self.dut.first_api_level)
        except ValueError as e:
            api_level = 0
            logging.exception(e)

        if api_level == 0:
            try:
                api_level = int(self.getProp("ro.build.version.sdk"))
            except ValueError as e:
                asserts.fail("Unexpected value returned from getprop: %s" % e)

        asserts.skipIf(
            int(api_level) <= api.PLATFORM_API_LEVEL_O_MR1,
            "Skip test for a device launched first before Android P.")

    def dtboPartitionPresent(self):
        """Checks for the presence of DTBO partition.

        Returns:
            Boolean, True if DTBO partition exists.
            string, path to read DTBO Image from.
        """
        try:
            slot_suffix = str(self.dut.getProp(PROPERTY_SLOT_SUFFIX))
        except ValueError as e:
            logging.exception(e)
            slot_suffix = ""
        dtbo_path = DTBO_PARTITION_PATH + slot_suffix
        partition_exists = target_file_utils.Exists(dtbo_path, self.shell)
        return partition_exists, dtbo_path

    def testCheckDTBOPartition(self):
        """Validates DTBO partition using mkdtboimg.py."""
        dtbo_exists, dtbo_path = self.dtboPartitionPresent()
        asserts.assertTrue(dtbo_exists, "DTBO Partition not present")
        logging.info("DTBO path %s", dtbo_path)
        self.adb.pull("%s %s/dtbo" % (dtbo_path, self.temp_dir))
        dtbo_dump_cmd = [
            "host/bin/mkdtboimg.py", "dump",
            "%s/dtbo" % self.temp_dir, "-b",
            "%s/dumped_dtbo" % self.temp_dir
        ]
        try:
            subprocess.check_call(dtbo_dump_cmd)
        except Exception as e:
            logging.exception(e)
            asserts.fail("Invalid DTBO Image")

    def testVerifyOverlay(self):
        """Verifies application of DT overlays."""
        overlay_idx_string = self.adb.shell(
            "cat /proc/cmdline | "
            "grep -o \"androidboot.dtbo_idx=[^ ]*\" |"
            "cut -d \"=\" -f 2")
        asserts.assertNotEqual(
            len(overlay_idx_string), 0,
            "Kernel command line missing androidboot.dtbo_idx")
        overlay_idx_list = overlay_idx_string.split(",")
        overlay_arg = []
        device_path = str(
            path_utils.JoinTargetPath(DEVICE_TEMP_DIR, self.abi_bitness))
        for idx in overlay_idx_list:
            overlay_file = "dumped_dtbo." + idx.rstrip()
            overlay_path = os.path.join(self.temp_dir, overlay_file)
            self.adb.push(overlay_path, device_path)
            overlay_arg.append(overlay_file)
        final_dt_path = path_utils.JoinTargetPath(device_path, "final_dt")
        self.shell.Execute("cp %s %s" % (FDT_PATH, final_dt_path))
        cd_cmd = "cd %s" % (device_path)
        verify_cmd = "./ufdt_verify_overlay final_dt %s" % (
            " ".join(overlay_arg))
        cmd = str("%s && %s" % (cd_cmd, verify_cmd))
        logging.info(cmd)
        results = self.shell.Execute(cmd)
        asserts.assertEqual(results[const.EXIT_CODE][0], 0,
                            "Incorrect Overlay Application")

    def tearDownClass(self):
        """Deletes temporary directories."""
        shutil.rmtree(self.temp_dir)
        device_path = str(
            path_utils.JoinTargetPath(DEVICE_TEMP_DIR, self.abi_bitness))
        self.shell.Execute("rm -rf %s" % device_path)


if __name__ == "__main__":
    test_runner.main()
