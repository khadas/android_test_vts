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

LOCAL_MODULE_CLASS := FAKE
LOCAL_IS_HOST_MODULE := true
LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE):
	@echo "VTS host-driven test target: $(LOCAL_MODULE)"
	$(hide) touch $@

VTS_TESTCASES_OUT := $(HOST_OUT)/vts/android-vts/testcases

vts_config_file_src := test/vts/$(VTS_CONFIG_SRC_DIR)/$(LOCAL_MODULE).config

ifneq (,$(wildcard $(vts_config_file_src)))
vts_config_file := $(VTS_TESTCASES_OUT)/vts-config/$(VTS_CONFIG_SRC_DIR)/$(LOCAL_MODULE).config

$(vts_config_file): $(vts_config_file_src) | $(ACP)
	$(hide) mkdir -p $(dir $@)
	$(hide) $(ACP) -fp $< $@

vts: $(vts_config_file)
endif
