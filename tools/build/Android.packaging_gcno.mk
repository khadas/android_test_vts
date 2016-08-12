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
ifeq ($(strip $(my_module_multilib)), 32)
shared_lib := $(call intermediates-dir-for,SHARED_LIBRARIES,$(VTS_GCNO_MODULE),,,true)
else
shared_lib := $(call intermediates-dir-for,SHARED_LIBRARIES,$(VTS_GCNO_MODULE))
endif

base_src_paths := ${basename ${VTS_GCOV_SRC_CPP_FILES}}

obj_target_files := $(addsuffix .o, $(addprefix $(shared_lib)/,${base_src_paths}))
gcno_target_files := $(addsuffix .gcno, $(addprefix $(shared_lib)/,${base_src_paths}))
src_files := $(addprefix $(VTS_GCOV_SRC_DIR)/, $(VTS_GCOV_SRC_CPP_FILES))

VTS_TESTCASES_OUT := $(HOST_OUT)/vts/android-vts/testcases/coverage
vts_framework_lib_gcno_files := $(addsuffix .gcno, $(addprefix \
	$(VTS_TESTCASES_OUT)/$(LOCAL_MODULE)/, ${base_src_paths}))
vts_framework_lib_src_files := $(addprefix \
	$(VTS_TESTCASES_OUT)/$(LOCAL_MODULE)/, $(VTS_GCOV_SRC_CPP_FILES))
src_dest_gcno_tuples := $(join $(gcno_target_files), $(addprefix :,\
	$(vts_framework_lib_gcno_files)))
src_dest_src_tuples := $(join $(src_files), $(addprefix :, \
	$(vts_framework_lib_src_files)))

ifneq (,$(filter $(LOCAL_NATIVE_COVERAGE),true always))
$(gcno_target_files): $(obj_target_files)
vts: $(call copy-many-files, $(src_dest_src_tuples)) \
	$(call copy-many-files, $(src_dest_gcno_tuples))
endif
