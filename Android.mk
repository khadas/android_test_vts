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
	@mkdir -p $(dir $@)
	@rm -f $@.list
	$(hide) find test -name '*.py' -or -name '*.config' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C test -l $@.list
	@rm -f $@.list
	$(hide) rm -rf $(VTS_TESTCASES_OUT)/vts
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	$(hide) touch -f $(VTS_TESTCASES_OUT)/vts/__init__.py

define vts-get-package-paths
$(foreach pkg,$(1),$(VTS_TESTCASES_OUT)/$(pkg).apk)
endef

include $(BUILD_SYSTEM)/definitions.mk

define vts-copy-apk
$(hide) $(ACP) -fp out/target/product/$(TARGET_PRODUCT)/data/app/$(1)/$(1).apk $(VTS_TESTCASES_OUT)
endef

define vts-copy-bin
$(hide) $(ACP) -fp $(call intermediates-dir-for,EXECUTABLES,$(1),,,true)/$(2) $(VTS_TESTCASES_OUT)
$(hide) $(ACP) -fp $(call intermediates-dir-for,EXECUTABLES,$(1))/$(3) $(VTS_TESTCASES_OUT)
endef

define vts-copy-lib
$(hide) $(ACP) -fp $(call intermediates-dir-for,SHARED_LIBRARIES,$(1),,,true)/LINKED/$(1).so $(VTS_TESTCASES_OUT)/$(2).so
$(hide) $(ACP) -fp $(call intermediates-dir-for,SHARED_LIBRARIES,$(1))/LINKED/$(1).so $(VTS_TESTCASES_OUT)/$(3).so
endef

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
  # android.hardware.power@1.0 \

vts_test_bin_hal_packages := \
  # hidl-power.default \

.PHONY: vts
vts: $(VTS_PYTHON_ZIP) $(vts_apk_packages) $(vts_bin_packages) $(vts_lib_packages) $(vts_test_bin_packages) $(vts_test_lib_hidl_packages) $(vts_test_lib_hal_packages) $(vts_test_bin_hal_packages) | $(ACP)
	$(hide) mkdir -p $(HOST_OUT)/vts
	$(hide) mkdir -p $(HOST_OUT)/vts/android-vts
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(call vts-copy-apk,CtsVerifier)  # apks
	$(call vts-copy-apk,sl4a)
	$(call vts-copy-bin,vts_hal_agent,vts_hal_agent32,vts_hal_agent64)  # framework bins
	$(call vts-copy-bin,vtssysfuzzer,fuzzer32,fuzzer64)
	$(call vts-copy-bin,vts_shell_driver,vts_shell_driver32,vts_shell_driver64)
	$(call vts-copy-lib,libvts_interfacespecification,libvts_interfacespecification,libvts_interfacespecification64)  # framework libs
	$(call vts-copy-lib,libvts_drivercomm,libvts_drivercomm,libvts_drivercomm64)
	$(call vts-copy-lib,libvts_multidevice_proto,libvts_multidevice_proto,libvts_multidevice_proto64)
	$(call vts-copy-lib,libvts_profiling,libvts_profiling,libvts_profiling64)
	$(call vts-copy-lib,libvts_datatype,libvts_datatype,libvts_datatype64)
	$(call vts-copy-lib,libvts_common,libvts_common,libvts_common64)
	$(call vts-copy-lib,libvts_codecoverage,libvts_codecoverage,libvts_codecoverage64)
	$(call vts-copy-lib,libvts_measurement,libvts_measurement,libvts_measurement64)
	$(call vts-copy-bin,libhwbinder_benchmark,libhwbinder_benchmark32,libhwbinder_benchmark64)  # test bins
	$(call vts-copy-bin,libbinder_benchmark,libbinder_benchmark32,libbinder_benchmark64)
	$(call vts-copy-bin,28838221_poc,28838221_poc32,28838221_poc64)
	$(call vts-copy-bin,30149612_poc,30149612_poc32,30149612_poc64)
	$(call vts-copy-lib,libhwbinder,libhwbinder,libhwbinder64)  # test libs
	$(call vts-copy-lib,android.hardware.tests.libhwbinder@1.0,android.hardware.tests.libhwbinder@1.0,android.hardware.tests.libhwbinder@1.064)  # HAL libs
	$(call vts-copy-lib,android.hardware.tests.libbinder,android.hardware.tests.libbinder,android.hardware.tests.libbinder64)
	$(call vts-copy-lib,lights.bullhead-vts,lights.bullhead-vts,lights.bullhead-vts64)
	# TODO: enable when hidl-power is merged.
	# $(call vts-copy-lib,android.hardware.power@1.0,android.hardware.power@1.0,android.hardware.power@1.064)
	# $(call vts-copy-bin,hidl-power.default,hidl-power.default32,hidl-power.default64)  # HAL bins

endif # vts
endif # linux
