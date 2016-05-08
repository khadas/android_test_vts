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

LOCAL_SRC_FILES := \
  binder/VtsFuzzerBinderService.cpp \
  component_loader/DllLoader.cpp \
  fuzz_tester/FuzzerBase.cpp \
  fuzz_tester/FuzzerWrapper.cpp \
  proto/InterfaceSpecificationMessage.proto \
  specification_parser/InterfaceSpecificationParser.cpp \
  specification_parser/SpecificationBuilder.cpp \
  utils/InterfaceSpecUtil.cpp \

LOCAL_C_INCLUDES := \
  bionic \
  libcore \
  system/extras \
  external/protobuf/src \
  frameworks/native/include \
  system/core/include

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  libbinder \
  liblog \
  libdl \
  libandroid_runtime \

LOCAL_PROTOC_OPTIMIZE_TYPE := full

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_MODULE := libvts_common_host
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_HOST_OS := darwin linux

# Files needed for VTSC.
LOCAL_SRC_FILES := \
  proto/InterfaceSpecificationMessage.proto \
  specification_parser/InterfaceSpecificationParser.cpp \
  utils/InterfaceSpecUtil.cpp

LOCAL_C_INCLUDES := \
  bionic \
  libcore \
  system/extras \
  external/protobuf/src \

LOCAL_SHARED_LIBRARIES := \
  libprotobuf-cpp-full \

LOCAL_PROTOC_OPTIMIZE_TYPE := full

include $(BUILD_HOST_SHARED_LIBRARY)
