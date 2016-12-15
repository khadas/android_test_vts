#
# Copyright 2015 The Android Open Source Project
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

# This contains the module build definitions for the hardware-specific
# components for this device.
#
# As much as possible, those components should be built unconditionally,
# with device-specific names to avoid collisions, to avoid device-specific
# bitrot and build breakages. Building a component unconditionally does
# *not* include it on all devices, so it is safe even with hardware-specific
# components.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lights.bullhead-vts
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := lights.c
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := \
    liblog \

# enable below for gcov
LOCAL_SANITIZE := never
LOCAL_CLANG := true
LOCAL_CFLAGS += -fprofile-arcs -ftest-coverage
LOCAL_LDFLAGS += --coverage

# enable below for sancov
#LOCAL_ARM_MODE := arm
#LOCAL_CLANG := true
#LOCAL_CFLAGS += -fsanitize=address -fsanitize-coverage=bb -fno-omit-frame-pointer
#LOCAL_LDFLAGS += -fsanitize=address -fsanitize-coverage=bb
#LOCAL_ADDRESS_SANITIZER := true

LOCAL_MULTILIB := both

LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_SHARED_LIBRARY)
include $(LOCAL_PATH)/../../../tools/build/Android.packaging_sharedlib.mk

VTS_GCNO_FILE := lights
include $(LOCAL_PATH)/../../../tools/build/Android.packaging_gcno.mk
