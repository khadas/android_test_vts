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

LOCAL_MODULE := vtsc
LOCAL_MODULE_HOST_OS := darwin linux

LOCAL_CPP_EXTENSION := .cpp
LOCAL_SRC_FILES := \
  VtsCompilerMain.cpp \

LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/android \
  $(LOCAL_PATH)/src \
  test/vts/sysfuzzer/common \

LOCAL_STATIC_LIBRARIES += \
  libz \

LOCAL_SHARED_LIBRARIES := \
  libprotobuf-cpp-full \
  libvts_common_host \

LOCAL_CFLAGS := -Wall -Werror

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,dist_files sdk,$(LOCAL_BUILT_MODULE))

include $(call all-makefiles-under,$(LOCAL_PATH))
