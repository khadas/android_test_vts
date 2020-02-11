#
# Copyright (C) 2020 The Android Open Source Project
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

-include external/linux-kselftest/android/kselftest_test_list.mk
-include external/ltp/android/ltp_package_list.mk

VTS_CORE_OUT_ROOT := $(HOST_OUT)/vts-core
VTS_CORE_TESTCASES_OUT := $(VTS_CORE_OUT_ROOT)/android-vts-core/testcases

build_list_dir := test/vts/tools/build/tasks/list
build_utils_dir := test/vts/tools/build/utils

include $(build_list_dir)/vts_apk_package_list.mk
include $(build_list_dir)/vts_bin_package_list.mk
include $(build_list_dir)/vts_lib_package_list.mk
include $(build_utils_dir)/vts_package_utils.mk

# Packaging rule for android-vts.zip's testcases dir (DATA subdir).
vts_target_native_modules := \
    $(vts_apk_packages) \
    $(vts_bin_packages) \
    $(vts_lib_packages)
vts_target_native_copy_pairs := \
  $(call target-native-copy-pairs,$(vts_target_native_modules),$(VTS_CORE_TESTCASES_OUT))

# Package vts-tradefed jars
test_suite_tools += $(HOST_OUT_JAVA_LIBRARIES)/vts-tradefed.jar \
    $(HOST_OUT_JAVA_LIBRARIES)/vts-tradefed-tests.jar

# Packaging rule for host-side Python logic, configs, and data files
host_framework_files := \
  $(call find-files-in-subdirs,test/vts,"*.py" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts,"*.runner_conf" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts,"*.push" -and -type f,.)
host_framework_copy_pairs := \
  $(foreach f,$(host_framework_files),\
    test/vts/$(f):$(VTS_CORE_TESTCASES_OUT)/vts/$(f))

# Packaging rule for android-vts.zip's testcases dir (DATA subdir).
# TODO(b/149249068): this should be fixed by packaging kernel tests as a standalone module like
# testcases/ltp and testcases/ltp64. Once such tests are no longer run through the python wrapper,
# we can stop packaging the kernel tests under testcases/DATA/nativetests(64)
target_native_modules := \
    $(kselftest_modules) \
    ltp \
    $(ltp_packages)

target_native_copy_pairs := \
  $(call target-native-copy-pairs,$(target_native_modules),$(VTS_CORE_TESTCASES_OUT))

# Only include test cases under kernel directory
host_testcase_files := \
  $(call find-files-in-subdirs,test/vts-testcase/kernel,"*.py" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts-testcase/kernel,"*.runner_conf" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts-testcase/kernel,"*.push" -and -type f,.) \
  $(call find-files-in-subdirs,test/vts-testcase/kernel,"*.dump" -and -type f,.)
host_testcase_copy_pairs := \
  $(foreach f,$(host_testcase_files),\
    test/vts-testcase/kernel/$(f):$(VTS_CORE_TESTCASES_OUT)/vts/testcases/kernel/$(f))

vts_test_artifact_paths := \
  $(call copy-many-files,$(host_framework_copy_pairs)) \
  $(call copy-many-files,$(target_native_copy_pairs)) \
  $(call copy-many-files,$(host_testcase_copy_pairs)) \
  $(call copy-many-files,$(vts_target_native_copy_pairs))

vts_target_native_modules :=
vts_target_native_copy_pairs :=
host_framework_files :=
host_framework_copy_pairs :=
target_native_modules :=
target_native_copy_pairs :=
host_testcase_files :=
host_testcase_copy_pairs :=