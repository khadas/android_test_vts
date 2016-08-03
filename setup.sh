#!/bin/bash

function vts_multidevice_target_setup {
  DEVICE=$1

  adb root
  adb shell mkdir /data/local/tmp/32
  adb shell mkdir /data/local/tmp/64
  adb push images/${DEVICE}/32/fuzzer32 /data/local/tmp/32/fuzzer32
  adb push images/${DEVICE}/64/fuzzer64 /data/local/tmp/64/fuzzer64
  adb push images/${DEVICE}/32/vts_shell_driver32 /data/local/tmp/32/vts_shell_driver32
  adb push images/${DEVICE}/64/vts_shell_driver64 /data/local/tmp/64/vts_shell_driver64
  adb push images/${DEVICE}/32/vts_hal_agent32 /data/local/tmp/32/vts_hal_agent32
  adb push images/${DEVICE}/64/vts_hal_agent64 /data/local/tmp/64/vts_hal_agent64

  echo "install vts framework packages"
  adb shell mkdir /data/local/tmp/32/hal
  adb shell mkdir /data/local/tmp/64/hal
  adb push images/${DEVICE}/32/libvts_common.so /data/local/tmp/32/libvts_common.so
  adb push images/${DEVICE}/64/libvts_common.so /data/local/tmp/64/libvts_common.so
  adb push images/${DEVICE}/32/libvts_interfacespecification.so /data/local/tmp/32/libvts_interfacespecification.so
  adb push images/${DEVICE}/64/libvts_interfacespecification.so /data/local/tmp/64/libvts_interfacespecification.so
  adb push images/${DEVICE}/32/libvts_drivercomm.so /data/local/tmp/32/libvts_drivercomm.so
  adb push images/${DEVICE}/64/libvts_drivercomm.so /data/local/tmp/64/libvts_drivercomm.so
  adb push images/${DEVICE}/32/libvts_datatype.so /data/local/tmp/32/libvts_datatype.so
  adb push images/${DEVICE}/64/libvts_datatype.so /data/local/tmp/64/libvts_datatype.so
  adb push images/${DEVICE}/32/libvts_measurement.so /data/local/tmp/32/libvts_measurement.so
  adb push images/${DEVICE}/64/libvts_measurement.so /data/local/tmp/64/libvts_measurement.so
  adb push images/${DEVICE}/32/libvts_codecoverage.so /data/local/tmp/32/libvts_codecoverage.so
  adb push images/${DEVICE}/64/libvts_codecoverage.so /data/local/tmp/64/libvts_codecoverage.so
  adb push images/${DEVICE}/32/libvts_multidevice_proto.so /data/local/tmp/32/libvts_multidevice_proto.so
  adb push images/${DEVICE}/64/libvts_multidevice_proto.so /data/local/tmp/64/libvts_multidevice_proto.so
  # hal
  echo "install hal packages"
  adb push images/${DEVICE}/32/hal/lights.${DEVICE}-vts.so /data/local/tmp/32/hal/lights.${DEVICE}-vts.so
  adb push images/${DEVICE}/64/hal/lights.${DEVICE}-vts.so /data/local/tmp/64/hal/lights.${DEVICE}-vts.so
  adb push images/${DEVICE}/32/hal/camera.${DEVICE}-vts.so /data/local/tmp/32/hal/camera.${DEVICE}-vts.so
  adb push images/${DEVICE}/32/hal/libmmcamera_interface.vts.so /data/local/tmp/32/libmmcamera_interface.vts.so
  adb push images/${DEVICE}/32/hal/libmmjpeg_interface.vts.so /data/local/tmp/32/libmmjpeg_interface.vts.so
  adb push images/${DEVICE}/32/hal/libqdMetaData.vts.so /data/local/tmp/32/libqdMetaData.vts.so
  adb push images/${DEVICE}/64/hal/libqdMetaData.vts.so /data/local/tmp/64/libqdMetaData.vts.so
  adb push images/${DEVICE}/32/hal/libqomx_core.vts.so /data/local/tmp/32/libqomx_core.vts.so

  # uncomment for hidl
  # adb push images/${DEVICE}/32/libhwbinder.so /data/local/tmp/32/libhwbinder.so
  # adb push images/${DEVICE}/64/libhwbinder.so /data/local/tmp/64/libhwbinder.so

  adb shell mkdir /data/local/tmp/spec
  adb push specification/hal_conventional/CameraHalV2.vts /data/local/tmp/spec/CameraHalV2.vts
  adb push specification/hal_conventional/CameraHalV2hw_device_t.vts /data/local/tmp/spec/CameraHalV2hw_device_t.vts
  adb push specification/hal_conventional/GpsHalV1.vts /data/local/tmp/spec/GpsHalV1.vts
  adb push specification/hal_conventional/GpsHalV1GpsInterface.vts /data/local/tmp/spec/GpsHalV1GpsInterface.vts
  adb push specification/hal_conventional/LightHalV1.vts /data/local/tmp/spec/LightHalV1.vts
  adb push specification/hal_conventional/WifiHalV1.vts /data/local/tmp/spec/WifiHalV1.vts
  adb push specification/hal_hidl/Nfc.vts /data/local/tmp/spec/Nfc.vts
  adb push specification/hal_hidl/NfcClientCallback.vts /data/local/tmp/spec/NfcClientCallback.vts
  adb push specification/lib_bionic/libmV1.vts /data/local/tmp/spec/libmV1.vts

  echo "install asan packages"
  # asan
  adb push ../../prebuilts/clang/host/linux-x86/clang-2812033/lib64/clang/3.8/lib/linux/libclang_rt.asan-aarch64-android.so /data/local/tmp/libclang_rt.asan-aarch64-android.so

  adb shell chmod 755 /data/local/tmp/32/fuzzer32
  adb shell chmod 755 /data/local/tmp/64/fuzzer64
  adb shell chmod 755 /data/local/tmp/32/vts_shell_driver32
  adb shell chmod 755 /data/local/tmp/64/vts_shell_driver64
  adb shell chmod 755 /data/local/tmp/32/vts_hal_agent32
  adb shell chmod 755 /data/local/tmp/64/vts_hal_agent64
}
