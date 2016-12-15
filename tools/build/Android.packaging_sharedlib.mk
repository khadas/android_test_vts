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

VTS_TESTCASES_OUT := $(HOST_OUT)/vts/android-vts/testcases

ifeq ($(LOCAL_32_BIT_ONLY),true)
vts_framework_lib_file := $(VTS_TESTCASES_OUT)/$(LOCAL_MODULE).so
else
vts_framework_lib_file := $(VTS_TESTCASES_OUT)/$(LOCAL_MODULE)64.so
endif

ifeq ($(strip $(my_module_multilib)), 32)
shared_lib := $(call intermediates-dir-for,SHARED_LIBRARIES,$(LOCAL_MODULE),,,true)
else
shared_lib := $(call intermediates-dir-for,SHARED_LIBRARIES,$(LOCAL_MODULE))
endif

$(vts_framework_lib_file): $(shared_lib)/LINKED/$(LOCAL_MODULE).so | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

vts: $(vts_framework_lib_file)

