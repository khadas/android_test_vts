LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := SampleCameraV2Test
VTS_CONFIG_SRC_DIR := testcases/host/camera/conventional/2_1
include test/vts/tools/build/Android.host_config.mk
