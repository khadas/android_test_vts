LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SampleLightTest
VTS_CONFIG_SRC_DIR := testcases/host/light/conventional
include test/vts/tools/build/Android.host_config.mk
