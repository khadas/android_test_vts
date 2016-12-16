LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := VibratorHidlTest
VTS_CONFIG_SRC_DIR := testcases/hal/vibrator/hidl
include test/vts/tools/build/Android.host_config.mk
