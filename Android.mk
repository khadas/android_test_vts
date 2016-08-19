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

LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)

ifeq ($(HOST_OS),linux)
ifneq ($(filter vts, $(MAKECMDGOALS)),)

include $(CLEAR_VARS)

VTS_PYTHON_ZIP := $(HOST_OUT)/vts_runner_python/vts_runner_python.zip

$(VTS_PYTHON_ZIP): test/vts/setup.py
	@echo "build vts python package: $(VTS_PYTHON_ZIP)"
	@mkdir -p $(dir $@)
	@rm -f $@.list
	$(hide) find test -name '*.py' -or -name '*.config' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C test -l $@.list
	@rm -f $@.list
	$(hide) rm -rf $(HOST_OUT)/vts/android-vts/testcases/vts
	$(hide) unzip $@ -d $(HOST_OUT)/vts/android-vts/testcases
	$(hide) touch -f $(HOST_OUT)/vts/android-vts/testcases/vts/__init__.py

.PHONY: vts_runner_python
vts_runner_python: $(VTS_PYTHON_ZIP)
vts: $(VTS_PYTHON_ZIP)

$(call dist-for-goals,vts,$(VTS_PYTHON_ZIP))

endif # vts
endif # linux
