LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SampleCameraV3Test
VTS_CONFIG_SRC_DIR := testcases/host/camera/conventional/3_4
include test/vts/tools/build/Android.host_config.mk
