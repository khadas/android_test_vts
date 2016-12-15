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

include $(LOCAL_PATH)/list/vts_apk_package_list.mk
include $(LOCAL_PATH)/list/vts_bin_package_list.mk
include $(LOCAL_PATH)/list/vts_lib_package_list.mk
include $(LOCAL_PATH)/list/vts_spec_file_list.mk
include $(LOCAL_PATH)/list/vts_test_bin_package_list.mk
include $(LOCAL_PATH)/list/vts_test_lib_package_list.mk
include $(LOCAL_PATH)/list/vts_test_lib_hal_package_list.mk
include $(LOCAL_PATH)/list/vts_test_lib_hidl_package_list.mk

# Packaging rule for android-vts.zip
test_suite_name := vts
test_suite_tradefed := vts-tradefed
test_suite_readme := test/vts/README.md

include $(BUILD_SYSTEM)/tasks/tools/compatibility.mk

.PHONY: vts
vts: $(compatibility_zip)
$(call dist-for-goals, vts, $(compatibility_zip))

# Packaging rule for android-vts.zip's testcases dir (DATA subdir).

my_modules := \
    $(vts_apk_packages) \
    $(vts_bin_packages) \
    $(vts_lib_packages) \
    $(vts_test_bin_packages) \
    $(vts_test_lib_packages) \
    $(vts_test_lib_hal_packages) \
    $(vts_test_lib_hidl_packages) \

my_copy_pairs :=
  $(foreach m,$(my_modules),\
    $(eval _built_files := $(strip $(ALL_MODULES.$(m).BUILT_INSTALLED)\
    $(ALL_MODULES.$(m)$(TARGET_2ND_ARCH_MODULE_SUFFIX).BUILT_INSTALLED)))\
    $(foreach i, $(_built_files),\
      $(eval bui_ins := $(subst :,$(space),$(i)))\
      $(eval ins := $(word 2,$(bui_ins)))\
      $(if $(filter $(TARGET_OUT_ROOT)/%,$(ins)),\
        $(eval bui := $(word 1,$(bui_ins)))\
        $(eval my_built_modules += $(bui))\
        $(eval my_copy_dest := $(patsubst data/%,DATA/%,\
                                 $(patsubst system/%,DATA/%,\
                                   $(patsubst $(PRODUCT_OUT)/%,%,$(ins)))))\
        $(eval my_copy_pairs += $(bui):$(VTS_TESTCASES_OUT)/$(my_copy_dest)))\
    ))

# Packaging rule for android-vts.zip's testcases dir (spec subdir).

my_spec_modules := \
    $(vts_spec_file_list) \

my_spec_copy_pairs :=
  $(foreach m,$(my_spec_modules),\
    $(eval my_spec_copy_pairs += $(m):$(VTS_TESTCASES_OUT)/spec/$(m)))\


$(compatibility_zip): $(call copy-many-files,$(my_copy_pairs)) $(call copy-many-files,$(my_spec_copy_pairs))
