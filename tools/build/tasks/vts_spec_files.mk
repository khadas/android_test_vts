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

LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/list/vts_spec_file_list.mk

my_modules := \
    $(vts_spec_file_list) \

my_copy_pairs :=
  $(foreach m,$(my_modules),\
    $(eval my_copy_pairs += $(m):$(VTS_TESTCASES_OUT)/spec/$(m)))\

.PHONY: vts
vts: $(call copy-many-files,$(my_copy_pairs))
	@echo "vts_spec_files $(my_modules)"
	@echo "vts_spec_files copy_pairs $(my_copy_pairs)"
