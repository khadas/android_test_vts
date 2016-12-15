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

vtslib_interfacespec_srcfiles := \
  hal_conventional/CameraHalV2.vts \
  hal_conventional/CameraHalV2hw_device_t.vts \
  hal_conventional/GpsHalV1.vts \
  hal_conventional/GpsHalV1GpsInterface.vts \
  hal_conventional/LightHalV1.vts \
  hal_conventional/WifiHalV1.vts \
  lib_bionic/libmV1.vts \

vtslib_interfacespec_includes := \
  $(LOCAL_PATH) \
  test/vts/sysfuzzer \
  test/vts/sysfuzzer/framework \
  test/vts/sysfuzzer/libdatatype \
  test/vts/sysfuzzer/libmeasurement \
  test/vts/sysfuzzer/common \
  bionic \
  libcore \
  device/google/gce/include \
  system/extras \
  system/media/camera/include \
  external/protobuf/src \
  external/libedit/src \
  $(TARGET_OUT_HEADERS) \

vtslib_interfacespec_shared_libraries := \
  libcutils \
  liblog \
  libdl \
  libandroid_runtime \
  libvts_datatype \
  libvts_common \
  libvts_measurement \
  libvts_multidevice_proto \
  libprotobuf-cpp-full \

vtslib_interfacespec_static_libraries := \
  libelf \

include $(CLEAR_VARS)

LOCAL_MODULE := libvts_interfacespecification
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := ${vtslib_interfacespec_srcfiles}
LOCAL_C_INCLUDES := ${vtslib_interfacespec_includes}
LOCAL_SHARED_LIBRARIES := ${vtslib_interfacespec_shared_libraries}
LOCAL_STATIC_LIBRARIES := ${vtslib_interfacespec_static_libraries}

LOCAL_PROTOC_OPTIMIZE_TYPE := full

LOCAL_MULTILIB := both

LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_SHARED_LIBRARY)
include $(LOCAL_PATH)/../tools/build/Android.packaging_sharedlib.mk

include $(CLEAR_VARS)

VTS_TESTCASES_OUT := $(HOST_OUT)/vts/android-vts/testcases
vts_spec_file1 := $(VTS_TESTCASES_OUT)/CameraHalV2.vts
vts_spec_file2 := $(VTS_TESTCASES_OUT)/CameraHalV2hw_device_t.vts
vts_spec_file3 := $(VTS_TESTCASES_OUT)/GpsHalV1.vts
vts_spec_file4 := $(VTS_TESTCASES_OUT)/GpsHalV1GpsInterface.vts
vts_spec_file5 := $(VTS_TESTCASES_OUT)/LightHalV1.vts
vts_spec_file6 := $(VTS_TESTCASES_OUT)/WifiHalV1.vts
vts_spec_file7 := $(VTS_TESTCASES_OUT)/Nfc.vts
vts_spec_file8 := $(VTS_TESTCASES_OUT)/NfcClientCallback.vts

$(vts_spec_file1): $(LOCAL_PATH)/hal_conventional/CameraHalV2.vts | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

$(vts_spec_file2): $(LOCAL_PATH)/hal_conventional/CameraHalV2hw_device_t.vts | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

$(vts_spec_file3): $(LOCAL_PATH)/hal_conventional/GpsHalV1.vts | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

$(vts_spec_file4): $(LOCAL_PATH)/hal_conventional/GpsHalV1GpsInterface.vts | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

$(vts_spec_file5): $(LOCAL_PATH)/hal_conventional/LightHalV1.vts | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

$(vts_spec_file6): $(LOCAL_PATH)/hal_conventional/WifiHalV1.vts | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

$(vts_spec_file7): $(LOCAL_PATH)/hal_hidl/Nfc.vts | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

$(vts_spec_file8): $(LOCAL_PATH)/hal_hidl/NfcClientCallback.vts | $(ACP)
	$(hide) mkdir -p $(VTS_TESTCASES_OUT)
	$(hide) $(ACP) -fp $< $@

vts: $(vts_spec_file1) $(vts_spec_file2) $(vts_spec_file3) $(vts_spec_file4) $(vts_spec_file5) $(vts_spec_file6) $(vts_spec_file7) $(vts_spec_file8)

