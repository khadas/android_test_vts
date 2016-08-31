OMX_CORE_PATH := $(call my-dir)

# ------------------------------------------------------------------------------
#                Make the shared library (libqomx_core)
# ------------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_PATH := $(OMX_CORE_PATH)
LOCAL_MODULE_TAGS := optional

# removed -Werror flag.
omx_core_defines:= -g -O0

LOCAL_CFLAGS := $(omx_core_defines)

OMX_HEADER_DIR := frameworks/native/include/media/openmax

LOCAL_C_INCLUDES := $(OMX_HEADER_DIR)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../qexif

LOCAL_SRC_FILES := qomx_core.c

# enable below for gcov
LOCAL_CLANG := true
LOCAL_CFLAGS += -fprofile-arcs -ftest-coverage
LOCAL_LDFLAGS += --coverage

LOCAL_MODULE           := libqomx_core.vts
LOCAL_PRELINK_MODULE   := false
LOCAL_SHARED_LIBRARIES := libcutils libdl

LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)

