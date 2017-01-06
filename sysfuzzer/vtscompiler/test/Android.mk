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

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# TODO(yim): uncomment this target after the input files are on AOSP.
#LOCAL_MODULE := vtsc_test
#LOCAL_MODULE_CLASS := FAKE
#LOCAL_IS_HOST_MODULE := true

#include $(BUILD_SYSTEM)/base_rules.mk

#the_py_script := $(LOCAL_PATH)/test_vtsc.py
#$(LOCAL_BUILT_MODULE): PRIVATE_PY_SCRIPT := $(the_py_script)
#$(LOCAL_BUILT_MODULE): PRIVATE_OUT_DIR := $(LOCAL_PATH)/test_out
#$(LOCAL_BUILT_MODULE): PRIVATE_CANONICAL_DIR := test/vts/sysfuzzer/vtscompiler/test/golden
#$(LOCAL_BUILT_MODULE): PRIVATE_HIDL_EXEC := $(HOST_OUT_EXECUTABLES)/vtsc
#$(LOCAL_BUILT_MODULE): $(the_py_script) $(HOST_OUT_EXECUTABLES)/vtsc
#	@echo "host Test: $(PRIVATE_MODULE)"
#	$(hide) PYTHONPATH=$$PYTHONPATH:test/vts/.. \
#	python $(PRIVATE_PY_SCRIPT)  -p $(PRIVATE_HIDL_EXEC) -c $(PRIVATE_CANONICAL_DIR) -o $(PRIVATE_OUT_DIR)
#	$(hide) touch $@
