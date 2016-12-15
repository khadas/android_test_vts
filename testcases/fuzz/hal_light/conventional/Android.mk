LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := LightFuzzTest
VTS_CONFIG_SRC_DIR := testcases/fuzz/hal_light/conventional
include test/vts/tools/build/Android.host_config.mk
