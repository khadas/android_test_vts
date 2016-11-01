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

hal_common_fuzz_dir := test/vts/testcases/hal/common/fuzz

module_name := $(module_name)_blind
module_path := hal_fuzz

LOCAL_MODULE := $(module_name)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_NATIVE_TESTS)/$(module_path)
LOCAL_SRC_FILES := $(module_src_files)

LOCAL_C_INCLUDES := \
    external/llvm/lib/Fuzzer \
    test/vts/testcases/hal/common/fuzz \

LOCAL_SHARED_LIBRARIES := \
    $(module_shared_libraries) \
    libutils \
    libhardware \

LOCAL_ARM_MODE := arm
LOCAL_CLANG := true
LOCAL_CFLAGS += \
    $(module_cflags) \
    -Wno-unused-parameter \
    -fno-omit-frame-pointer \

LOCAL_STATIC_LIBRARIES := \
    libLLVMFuzzer \

LOCAL_SANITIZE := address coverage

include $(BUILD_EXECUTABLE)

# Copy resulting executable to vts directory.
include $(hal_common_fuzz_dir)/Android.vts_testcase.mk

module_name :=
module_path :=
