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

gcno_obj_file := $(call intermediates-dir-for,SHARED_LIBRARIES,$(VTS_GCNO_MODULE))/${VTS_GCNO_FILE}.o
gcno_src_file := $(call intermediates-dir-for,SHARED_LIBRARIES,$(VTS_GCNO_MODULE))/${VTS_GCNO_FILE}.gcno

VTS_TESTCASES_OUT := $(HOST_OUT)/vts/android-vts/testcases

vts_framework_lib_gcno_file := $(VTS_TESTCASES_OUT)/${LOCAL_MODULE}_${VTS_GCNO_FILE}.gcno

$(vts_framework_lib_gcno_file): $(gcno_obj_file) | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) touch $(gcno_src_file)
	$(hide) $(ACP) -fp $(gcno_src_file) $@

vts_framework_lib_src_file := $(VTS_TESTCASES_OUT)/${LOCAL_MODULE}_${VTS_GCOV_SRC_CPP_FILE}

$(vts_framework_lib_src_file): ${VTS_GCOV_SRC_DIR}/${VTS_GCOV_SRC_CPP_FILE} | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

vts: $(vts_framework_lib_gcno_file) $(vts_framework_lib_src_file)
