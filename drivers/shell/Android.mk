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

include $(CLEAR_VARS)
LOCAL_MODULE := vts_shell_driver
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  shell_msg_protocol.cpp \
  shell_driver.cpp \
  shell_driver_main.cpp \

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \

LOCAL_CFLAGS := $(common_c_flags)
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := vts_shell_driver_test
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  shell_msg_protocol.cpp \
  shell_driver.cpp \
  shell_driver_test_client.cpp \
  shell_driver_test.cpp \

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \

LOCAL_CFLAGS := $(common_c_flags)
include $(BUILD_NATIVE_TEST)