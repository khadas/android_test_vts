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
VTS_OUT_ROOT := $(HOST_OUT)/vts
VTS_CAMERAITS_ZIP := $(VTS_OUT_ROOT)/CameraITS.zip
VTS_TESTCASES_OUT := $(VTS_OUT_ROOT)/android-vts/testcases

.PHONY: $(VTS_PYTHON_ZIP) $(VTS_CAMERAITS_ZIP)
$(VTS_PYTHON_ZIP): $(SOONG_ZIP)
	@echo "build vts python package: $(VTS_PYTHON_ZIP)"
	$(hide) mkdir -p $(dir $@)
	$(hide) rm -rf $(VTS_TESTCASES_OUT)/vts
	$(hide) find test/vts -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C test -l $@.list
	@rm -f $@.list
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
	@echo "build vts python testcases"
	$(hide) find test/vts-testcase -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C test/vts-testcase -l $@.list
	@rm -f $@.list
	$(hide)unzip -o $@ -d $(VTS_TESTCASES_OUT)/vts/testcases/
	#
	@echo "build vts python package for audio effect HAL"
	$(hide) find hardware/interfaces/audio/effect/2.0/vts/functional -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	# uncomment when audio effect HAL test has some py, config, or push files
	# $(hide) $(SOONG_ZIP) -d -o $@ -C hardware/interfaces/audio/effect/2.0/vts/functional -l $@.list
	@rm -f $@.list
	# $(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
	@echo "build vts python package for NFC HAL"
	$(hide) find hardware/interfaces/nfc/1.0/vts/functional -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C hardware/interfaces/nfc/1.0/vts/functional -l $@.list
	@rm -f $@.list
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
	@echo "build vts python package for power HAL"
	$(hide) find hardware/interfaces/power/1.0/vts/functional -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C hardware/interfaces/power/1.0/vts/functional -l $@.list
	@rm -f $@.list
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
	@echo "build vts python package for thermal HAL"
	$(hide) find hardware/interfaces/thermal/1.0/vts/functional -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C hardware/interfaces/thermal/1.0/vts/functional -l $@.list
	@rm -f $@.list
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
	@echo "build vts python package for vibrator HAL"
	$(hide) find hardware/interfaces/vibrator/1.0/vts/functional -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C hardware/interfaces/vibrator/1.0/vts/functional -l $@.list
	@rm -f $@.list
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
	@echo "build vts python package for sensors HAL"
	$(hide) find hardware/interfaces/sensors/1.0/vts/functional -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C hardware/interfaces/sensors/1.0/vts/functional -l $@.list
	@rm -f $@.list
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
	@echo "build vts python package for VR HAL"
	$(hide) find hardware/interfaces/vr/1.0/vts/functional -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C hardware/interfaces/vr/1.0/vts/functional -l $@.list
	@rm -f $@.list
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
	@echo "build vts python package for tv hdmi_cec HAL"
	$(hide) find hardware/interfaces/tv/cec/1.0/vts/functional -name '*.py' -or -name '*.config' -or -name '*.push' | sort > $@.list
	$(hide) $(SOONG_ZIP) -d -o $@ -C hardware/interfaces/tv/cec/1.0/vts/functional -l $@.list
	@rm -f $@.list
	$(hide) unzip $@ -d $(VTS_TESTCASES_OUT)
	#
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

my_deps_copy_pairs :=
  $(foreach d,$(ADDITIONAL_VTS_JARS),\
    $(eval my_deps_copy_pairs += $(d):$(VTS_OUT_ROOT)/android-vts/tools/$(notdir $(d))))

vts: $(VTS_PYTHON_ZIP) $(VTS_CAMERAITS_ZIP) $(call copy-many-files,$(my_deps_copy_pairs))

endif # vts
endif # linux
