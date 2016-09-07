#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Initiate a test case directory.
# This script copy a template which contains Android.mk, __init__.py files,
# AndroidTest.xml and a test case python file into a given relative directory
# under testcases/ using the given test name.

LICENSE_STATEMENT_POUND = '''#
# Copyright (C) {year} The Android Open Source Project
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
'''

LICENSE_STATEMENT_XML = '''<!-- Copyright (C) {year} The Android Open Source Project

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

          http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
'''

ANDROID_MK_TEMPLATE = '''LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)

include $(CLEAR_VARS)

LOCAL_MODULE := {test_name}
LOCAL_MODULE_CLASS := FAKE
LOCAL_IS_HOST_MODULE := true
LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE):
\t@echo "VTS host-driven test target: $(LOCAL_MODULE)"
\t$(hide) touch $@

VTS_CONFIG_SRC_DIR := {config_src_dir}
include test/vts/tools/build/Android.host_config.mk
'''

ANDROID_MK_CALL_SUB = '''LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)
'''

XML_HEADER = '''<?xml version="1.0" encoding="utf-8"?>
'''

ANDROID_TEST_XML_TEMPLATE = '''<configuration description="Config for VTS {test_name} test cases">
    <target_preparer class="com.android.compatibility.common.tradefed.targetprep.VtsFilePusher">
        <option name="push-group" value="{test_type}.push" />
    </target_preparer>
    <target_preparer class="com.android.tradefed.targetprep.VtsPythonVirtualenvPreparer">
    </target_preparer>
    <test class="com.android.tradefed.testtype.VtsMultiDeviceTest">
        <option name="test-case-path" value="vts/{test_path_under_vts}/{test_case_file_without_extension}" />
    </test>
</configuration>
'''

PY_HEADER = '''#!/usr/bin/env python
'''

TEST_CASE_PY_TEMPLATE = '''import logging

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class {test_name}(base_test_with_webdb.BaseTestWithWebDbClass):
    """Two hello world test cases which use the shell driver."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]

    def testEcho1(self):
        """A simple testcase which sends a command."""
        self.dut.shell.InvokeTerminal("my_shell1")  # creates a remote shell instance.
        results = self.dut.shell.my_shell1.Execute("echo hello_world")  # runs a shell command.
        logging.info(str(results[const.STDOUT]))  # prints the stdout
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello_world")  # checks the stdout
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)  # checks the exit code

    def testEcho2(self):
        """A simple testcase which sends two commands."""
        self.dut.shell.InvokeTerminal("my_shell2")
        my_shell = getattr(self.dut.shell, "my_shell2")
        results = my_shell.Execute(["echo hello", "echo world"])
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 2)  # check the number of processed commands
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello")
        asserts.assertEqual(results[const.STDOUT][1].strip(), "world")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)
        asserts.assertEqual(results[const.EXIT_CODE][1], 0)


if __name__ == "__main__":
    test_runner.main()
'''
