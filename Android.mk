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
	$(hide) cd test/vts; python setup.py sdist --formats=zip
	$(hide) unzip test/vts/dist/vts-0.1.zip -d $(HOST_OUT)/vts/android-vts/testcases
	$(hide) mv -f $(HOST_OUT)/vts/android-vts/testcases/vts-0.1 $(HOST_OUT)/vts/android-vts/testcases/vts
	$(hide) touch -f $(HOST_OUT)/vts/android-vts/testcases/vts/__init__.py
	$(hide) cp -f test/vts/dist/vts-0.1.zip $@

.PHONY: vts_runner_python
vts_runner_python: $(VTS_PYTHON_ZIP)
vts: $(VTS_PYTHON_ZIP)

$(call dist-for-goals,vts,$(VTS_PYTHON_ZIP))

# TODO (sahjain) Include java build script (from buil-python.sh) in .mk file

endif # vts
endif # linux
