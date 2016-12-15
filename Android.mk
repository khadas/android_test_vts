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
VTS_TESTCASES_OUT := $(HOST_OUT)/vts/android-vts/testcases

.PHONY: $(VTS_PYTHON_ZIP)
$(VTS_PYTHON_ZIP): $(SOONG_ZIP)
	@echo "build vts python package: $(VTS_PYTHON_ZIP)"
	$(hide) mkdir -p $(dir $@)
	@rm -f $@.list
	$(hide) find test -name '*.py' -or -name '*.config' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C test -l $@.list
	@rm -f $@.list
	$(hide) rm -rf $(VTS_TESTCASES_OUT)/vts
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	$(hide) touch -f $(VTS_TESTCASES_OUT)/vts/__init__.py

vts_apk_packages := \
  CtsVerifier \
  sl4a \

vts_bin_packages := \
  vts_hal_agent \
  vtssysfuzzer \
  vts_shell_driver \

vts_test_bin_packages := \
  libhwbinder_benchmark \
  libbinder_benchmark \
  28838221_poc \
  30149612_poc \

vts_lib_packages := \
  libvts_interfacespecification \
  libvts_drivercomm \
  libvts_multidevice_proto \
  libvts_profiling \
  libvts_datatype \
  libvts_common \
  libvts_codecoverage \
  libvts_measurement \

vts_test_lib_hidl_packages := \
  libhwbinder \

vts_test_lib_hal_packages := \
  android.hardware.tests.libhwbinder@1.0 \
  android.hardware.tests.libbinder \
  lights.bullhead-vts \

vts_test_bin_hal_packages := \

.PHONY: vts_runner_python
vts_runner_python: $(VTS_PYTHON_ZIP)

.PHONY: vts
vts: $(VTS_PYTHON_ZIP)

endif # vts
endif # linux
