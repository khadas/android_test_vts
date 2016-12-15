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
  specification/CameraHalV2.vts \
  specification/CameraHalV2hw_device_t.vts \
  specification/GpsHalV1.vts \
  specification/GpsHalV1GpsInterface.vts \
  specification/LightHalV1.vts \
  specification/WifiHalV1.vts \

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

include $(BUILD_SHARED_LIBRARY)
