#!/bin/bash

BASE_DIR=`pwd`/../..
echo $BASE_DIR

function vts_multidevice_create_image {
  DEVICE=$1
  BUILD_TARGET=$2

  . ${BASE_DIR}/build/envsetup.sh
  cd ${BASE_DIR}; lunch ${BUILD_TARGET}-userdebug
  cd ${BASE_DIR}/test/vts; mma -j 32 && cd ${BASE_DIR}; make vts -j 32

  mkdir -p ${BASE_DIR}/test/vts/images
  mkdir -p ${BASE_DIR}/test/vts/images/${DEVICE}
  mkdir -p ${BASE_DIR}/test/vts/images/${DEVICE}/32
  mkdir -p ${BASE_DIR}/test/vts/images/${DEVICE}/64
  mkdir -p ${BASE_DIR}/test/vts/images/${DEVICE}/32/hal
  mkdir -p ${BASE_DIR}/test/vts/images/${DEVICE}/64/hal
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/bin/fuzzer32 test/vts/images/${DEVICE}/32/fuzzer32 -f
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/bin/fuzzer64 test/vts/images/${DEVICE}/64/fuzzer64 -f
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/bin/vts_shell_driver32 test/vts/images/${DEVICE}/32/vts_shell_driver32 -f
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/bin/vts_shell_driver64 test/vts/images/${DEVICE}/64/vts_shell_driver64 -f
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/bin/vts_hal_agent32 test/vts/images/${DEVICE}/32/vts_hal_agent32 -f
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/bin/vts_hal_agent64 test/vts/images/${DEVICE}/64/vts_hal_agent64 -f

  # .so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libvts_common.so test/vts/images/${DEVICE}/32/libvts_common.so -f
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libvts_common.so test/vts/images/${DEVICE}/64/libvts_common.so -f
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libvts_datatype.so test/vts/images/${DEVICE}/32/libvts_datatype.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libvts_datatype.so test/vts/images/${DEVICE}/64/libvts_datatype.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libvts_drivercomm.so test/vts/images/${DEVICE}/32/libvts_drivercomm.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libvts_drivercomm.so test/vts/images/${DEVICE}/64/libvts_drivercomm.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libvts_interfacespecification.so test/vts/images/${DEVICE}/32/libvts_interfacespecification.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libvts_interfacespecification.so test/vts/images/${DEVICE}/64/libvts_interfacespecification.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libvts_measurement.so test/vts/images/${DEVICE}/32/libvts_measurement.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libvts_measurement.so test/vts/images/${DEVICE}/64/libvts_measurement.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libvts_codecoverage.so test/vts/images/${DEVICE}/32/libvts_codecoverage.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libvts_codecoverage.so test/vts/images/${DEVICE}/64/libvts_codecoverage.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libvts_multidevice_proto.so test/vts/images/${DEVICE}/32/libvts_multidevice_proto.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libvts_multidevice_proto.so test/vts/images/${DEVICE}/64/libvts_multidevice_proto.so
  # HAL
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/hw/lights.${DEVICE}-vts.so test/vts/images/${DEVICE}/32/hal/lights.${DEVICE}-vts.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/hw/lights.${DEVICE}-vts.so test/vts/images/${DEVICE}/64/hal/lights.${DEVICE}-vts.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/android.hardware.nfc@1.0.so test/vts/images/${DEVICE}/32/hal/android.hardware.nfc@1.0.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/android.hardware.nfc@1.0.so test/vts/images/${DEVICE}/64/hal/android.hardware.nfc@1.0.so

  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/hw/camera.${DEVICE}-vts.so test/vts/images/${DEVICE}/32/hal/camera.${DEVICE}-vts.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libmmcamera_interface.vts.so test/vts/images/${DEVICE}/32/hal/libmmcamera_interface.vts.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libmmjpeg_interface.vts.so test/vts/images/${DEVICE}/32/hal/libmmjpeg_interface.vts.so
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libqdMetaData.vts.so test/vts/images/${DEVICE}/32/hal
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libqdMetaData.vts.so test/vts/images/${DEVICE}/64/hal
  cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libqomx_core.vts.so test/vts/images/${DEVICE}/32/hal

  # uncomment for hidl
  # cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib/libhwbinder.so test/vts/images/${DEVICE}/32/libhwbinder.so
  # cp ${BASE_DIR}/out/target/product/${DEVICE}/system/lib64/libhwbinder.so test/vts/images/${DEVICE}/64/libhwbinder.so
}
