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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libvts_common
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -Wno-unused-parameter -Werror

LOCAL_SRC_FILES := \
  binder/VtsFuzzerBinderService.cpp \
  component_loader/DllLoader.cpp \
  fuzz_tester/FuzzerBase.cpp \
  fuzz_tester/FuzzerCallbackBase.cpp \
  fuzz_tester/FuzzerWrapper.cpp \
  specification_parser/InterfaceSpecificationParser.cpp \
  specification_parser/SpecificationBuilder.cpp \
  utils/InterfaceSpecUtil.cpp \

LOCAL_C_INCLUDES := \
  bionic \
  libcore \
  system/extras \
  external/protobuf/src \
  frameworks/native/include \
  test/vts/drivers/libdrivercomm \
  test/vts/sysfuzzer/libcodecoverage \
  system/core/include \

LOCAL_SHARED_LIBRARIES := \
  libbinder \
  libandroid_runtime \
  libvts_drivercomm \
  libvts_codecoverage \
  libvts_multidevice_proto \
  libprotobuf-cpp-full \

LOCAL_STATIC_LIBRARIES := \
  libutils \
  libcutils \
  liblog \
  libdl \

LOCAL_MULTILIB := both

LOCAL_COMPATIBILITY_SUITE := vts

include test/vts/tools/build/Android.packaging_sharedlib.mk
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libvts_common_host
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_HOST_OS := darwin linux

LOCAL_CFLAGS += -Wno-unused-parameter -Werror

# Files needed for VTSC.
LOCAL_SRC_FILES := \
  specification_parser/InterfaceSpecificationParser.cpp \
  utils/InterfaceSpecUtil.cpp

LOCAL_C_INCLUDES := \
  bionic \
  libcore \
  system/extras \
  external/protobuf/src \

LOCAL_SHARED_LIBRARIES := \
  libprotobuf-cpp-full \
  libvts_multidevice_proto_host \

include $(BUILD_HOST_SHARED_LIBRARY)
