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
LOCAL_MODULE_STEM_64 := vts_hal_agent64
LOCAL_MODULE_STEM_32 := vts_hal_agent32
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  VtsAgentMain.cpp \
  TcpServerForRunner.cpp \
  AgentRequestHandler.cpp \
  SocketClientToDriver.cpp \
  BinderClientToDriver.cpp \
  SocketServerForDriver.cpp \

LOCAL_SHARED_LIBRARIES := \
  libbinder \
  libprotobuf-cpp-full \
  libvts_common \
  libvts_multidevice_proto \
  libvts_drivercomm \

LOCAL_STATIC_LIBRARIES := \
  libc++_static \
  libutils \
  libcutils \


LOCAL_C_INCLUDES += \
  bionic \
  external/libcxx/include \
  frameworks/native/include \
  system/core/include \
  test/vts/sysfuzzer/common \
  test/vts/agents/hal \
  test/vts/agents/hal/proto \
  test/vts/drivers/libdrivercomm \
  external/protobuf/src \

LOCAL_MULTILIB := both

LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_EXECUTABLE)
