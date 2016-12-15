# This's a fork of the camera HAL code under /device/lge/bullhead and is forked
# to enable the instrumentations (e.g., gcov) necessary for VTS.
ifneq ($(filter msm8992 msm8994,$(TARGET_BOARD_PLATFORM)),)
include $(call all-subdir-makefiles)
endif
