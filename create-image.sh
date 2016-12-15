#!/bin/bash

function vts_multidevice_create_image {
  DEVICE=$1

  . ${ANDROID_BUILD_TOP}/build/envsetup.sh
  cd ${ANDROID_BUILD_TOP}; lunch ${DEVICE}-userdebug
  cd ${ANDROID_BUILD_TOP}/test/vts; mma -j 32 && cd ${ANDROID_BUILD_TOP}; make vts adb -j 32
}
