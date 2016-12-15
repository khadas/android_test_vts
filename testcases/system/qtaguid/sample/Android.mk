LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SampleQtaguidTest
VTS_CONFIG_SRC_DIR := testcases/system/qtaguid/sample
include test/vts/tools/build/Android.host_config.mk
