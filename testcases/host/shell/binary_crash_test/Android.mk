LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := vts_test_binary_crash_app
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  crash_app.c \

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  liblog \

LOCAL_C_INCLUDES += \
  bionic \

LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := ShellBinaryCrashTest
LOCAL_MODULE_CLASS := FAKE
LOCAL_IS_HOST_MODULE := true
LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE):
	@echo "VTS host-driven test target: $(LOCAL_MODULE)"
	$(hide) touch $@

VTS_CONFIG_SRC_DIR := testcases/host/shell/binary_crash_test
include test/vts/tools/build/Android.host_config.mk
