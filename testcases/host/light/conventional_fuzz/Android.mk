LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SampleLightFuzzTest
LOCAL_MODULE_CLASS := FAKE
LOCAL_IS_HOST_MODULE := true
LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE):
	@echo "VTS host-driven test target: $(LOCAL_MODULE)"
	$(hide) touch $@

VTS_CONFIG_SRC_DIR := testcases/host/light/conventional_fuzz
include $(LOCAL_PATH)/../../../../tools/build/Android.host_config.mk
