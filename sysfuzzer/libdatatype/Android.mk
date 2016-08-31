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

LOCAL_MODULE := libvts_datatype
LOCAL_MODULE_TAGS := optional

# TODO: uncomment when ready
#LOCAL_CFLAGS += -Wno-unused-parameter -Werror

LOCAL_SRC_FILES := \
  vts_datatype.cpp \
  hal_light.cpp \
  hal_gps.cpp \
  hal_camera.cpp \

LOCAL_C_INCLUDES := \
  bionic \
  libcore \
  external/protobuf/src \
  libvts_common \
  system/media/camera/include \

LOCAL_SHARED_LIBRARIES := \
  libcutils \
  libvts_common \
  libvts_multidevice_proto \
  libprotobuf-cpp-full \

LOCAL_MULTILIB := both

include $(BUILD_SHARED_LIBRARY)
