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
  hal_conventional/CameraHalV3.vts \
  hal_conventional/CameraHalV3camera3_device_ops_t.vts \
  hal_conventional/GpsHalV1.vts \
  hal_conventional/GpsHalV1GpsInterface.vts \
  hal_conventional/LightHalV1.vts \
  hal_conventional/WifiHalV1.vts \
  hal_conventional/BluetoothHalV1.vts \
  hal_conventional/BluetoothHalV1bt_interface_t.vts \
  lib_bionic/libmV1.vts \
  lib_bionic/libcV1.vts \
  lib_bionic/libcutilsV1.vts \

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
  libcamera_metadata \
  libvts_datatype \
  libvts_common \
  libvts_measurement \
  libvts_multidevice_proto \
  libprotobuf-cpp-full \

vtslib_interfacespec_static_libraries := \

include $(CLEAR_VARS)

# libvts_interfacespecification does  include or link any HIDL HAL driver.
# HIDL HAL drivers and profilers are defined as separated shared libraries
# in a respective hardware/interfaces/<hal name>/<version>/Android.bp file.
# libvts_interfacespecification is the driver for:
#   legacy HALs,
#   conventional HALs,
#   shared libraries,
#   and so on.

LOCAL_MODULE := libvts_interfacespecification
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  ${vtslib_interfacespec_srcfiles} \

LOCAL_C_INCLUDES := \
  ${vtslib_interfacespec_includes} \
  system/core/base/include \

LOCAL_SHARED_LIBRARIES := \
  ${vtslib_interfacespec_shared_libraries} \

LOCAL_STATIC_LIBRARIES := \
  ${vtslib_interfacespec_static_libraries}

LOCAL_PROTOC_OPTIMIZE_TYPE := full

LOCAL_MULTILIB := both

include $(BUILD_SHARED_LIBRARY)
