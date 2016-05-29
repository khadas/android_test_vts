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

include $(CLEAR_VARS)

VTS_PYTHON_TGZ := $(HOST_OUT)/vts/vts.tar.gz

$(VTS_PYTHON_TGZ): test/vts/setup.py
	@echo "build vts python package: $(VTS_PYTHON_TGZ)"
	@mkdir -p $(dir $@)
	$(hide) cd test/vts; python setup.py sdist --formats=gztar
	$(hide) cp -f test/vts/dist/vts-0.1.tar.gz $@

vts: $(VTS_PYTHON_TGZ)

endif # linux
