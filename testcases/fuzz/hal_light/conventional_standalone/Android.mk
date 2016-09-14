LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := StandaloneLightFuzzTest
VTS_CONFIG_SRC_DIR := testcases/fuzz/hal_light/conventional_standalone
include test/vts/tools/build/Android.host_config.mk
