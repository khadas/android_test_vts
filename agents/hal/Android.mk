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

include $(CLEAR_VARS)

LOCAL_MODULE := vts_hal_agent
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  HalAgentMain.cpp \
  BinderClient.cpp \
  TcpServer.cpp \
  RequestHandler.cpp \

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  libbinder \
  libvts_common \
  libc++ \
  libvts_multidevice_proto \
  libprotobuf-cpp-full \

LOCAL_C_INCLUDES += \
  bionic \
  external/libcxx/include \
  frameworks/native/include \
  system/core/include \
  test/vts/sysfuzzer/common \
  test/vts/agents/hal \
  test/vts/agents/hal/proto \
  external/protobuf/src \

include $(BUILD_EXECUTABLE)
