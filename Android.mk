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
VTS_CAMERAITS_ZIP := $(HOST_OUT)/vts/CameraITS.zip
VTS_TESTCASES_OUT := $(HOST_OUT)/vts/android-vts/testcases

.PHONY: $(VTS_PYTHON_ZIP) $(VTS_CAMERAITS_ZIP)
$(VTS_PYTHON_ZIP): $(SOONG_ZIP)
	@echo "build vts python package: $(VTS_PYTHON_ZIP)"
	$(hide) mkdir -p $(dir $@)
	@rm -f $@.list
	$(hide) find test -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C test -l $@.list
	@rm -f $@.list
	$(hide) rm -rf $(VTS_TESTCASES_OUT)/vts
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	$(hide) touch -f $(VTS_TESTCASES_OUT)/vts/__init__.py

$(VTS_CAMERAITS_ZIP): $(SOONG_ZIP)
	@echo "build vts CameraITS package: $(VTS_CAMERAITS_ZIP)"
	$(hide) find cts/apps/CameraITS -name '*.py' -or -name '*.pdf' -or -name '*.png' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C cts/apps/CameraITS -l $@.list
	@rm -f $@.list
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)/CameraITS
	$(hide) rm -rf $(VTS_TESTCASES_OUT)/CameraITS/*
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)/CameraITS

.PHONY: vts
vts: $(VTS_PYTHON_ZIP) $(VTS_CAMERAITS_ZIP)

endif # vts
endif # linux
