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
from vts.utils.python.file import target_file_utils
from vts.utils.python.os import path_utils

DTBO_PARTITION_PATH = "/dev/block/bootdevice/by-name/dtbo"  # path to DTBO partition.
DEVICE_TEMP_DIR = "/data/local/tmp/"  # temporary dir in device.
FDT_PATH = "/sys/firmware/fdt"  # path to device tree.


class VtsDtboVerificationTest(base_test.BaseTestClass):
    """Validates DTBO partition and DT overlay application."""

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell
        self.adb = self.dut.adb
        self.temp_dir = tempfile.mkdtemp()
        logging.info("Create %s", self.temp_dir)
        device_path = str(
            path_utils.JoinTargetPath(DEVICE_TEMP_DIR, self.abi_bitness))
        self.shell.Execute("mkdir %s -p" % device_path)

    def testCheckDTBOPartition(self):
        """Validates DTBO partition using mkdtboimg.py."""
        asserts.skipIf("x86" in self.dut.cpu_abi, "Skipping test for x86 ABI")
        slot_suffix = str(self.dut.getProp("ro.boot.slot_suffix"))
        dtbo_path = DTBO_PARTITION_PATH + slot_suffix
        logging.info("DTBO path %s", dtbo_path)
        asserts.assertTrue(
            target_file_utils.Exists(dtbo_path, self.shell),
            "DTBO partition does not exist")
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
        asserts.skipIf("x86" in self.dut.cpu_abi, "Skipping test for x86 ABI")
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
        verify_cmd = "ufdt_verify_overlay final_dt %s" % (
            " ".join(overlay_arg))
        cmd = str("%s && %s" % (cd_cmd, verify_cmd))
        logging.info(cmd)
        results = self.shell.Execute(cmd)
        asserts.assertEqual(results[const.EXIT_CODE][0], 0,
                            "Incorrect Overlay Application")

    def tearDownClass(self):
        shutil.rmtree(self.temp_dir)
        device_path = str(
            path_utils.JoinTargetPath(DEVICE_TEMP_DIR, self.abi_bitness))
        self.shell.Execute("rm -rf %s" % device_path)


if __name__ == "__main__":
    test_runner.main()
