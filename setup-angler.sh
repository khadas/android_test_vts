#!/bin/bash

adb root
adb push images/angler/fuzzer /data/local/tmp/fuzzer
adb push images/angler/vts_hal_agent /data/local/tmp/vts_hal_agent
adb shell mkdir /data/local/tmp/lib
adb shell mkdir /data/local/tmp/hal
adb shell mkdir /data/local/tmp/hal64
adb push images/angler/libvts_common32.so /data/local/tmp/lib/libvts_common.so
adb push images/angler/libvts_common.so /data/local/tmp/libvts_common.so
adb push images/angler/libvts_interfacespecification32.so /data/local/tmp/lib/libvts_interfacespecification.so
adb push images/angler/libvts_interfacespecification.so /data/local/tmp/libvts_interfacespecification.so
adb push images/angler/libvts_datatype32.so /data/local/tmp/lib/libvts_datatype.so
adb push images/angler/libvts_datatype.so /data/local/tmp/libvts_datatype.so
adb push images/angler/libvts_measurement32.so /data/local/tmp/lib/libvts_measurement.so
adb push images/angler/libvts_measurement.so /data/local/tmp/libvts_measurement.so
adb push images/angler/libvts_codecoverage32.so /data/local/tmp/lib/libvts_codecoverage.so
adb push images/angler/libvts_codecoverage.so /data/local/tmp/libvts_codecoverage.so
# hal
adb push images/bullhead/hal/lights.bullhead-vts.so /data/local/tmp/hal/lights.bullhead-vts.so
adb push images/bullhead/hal64/lights.bullhead-vts.so /data/local/tmp/hal64/lights.bullhead-vts.so

adb shell mkdir /data/local/tmp/spec
adb push sysfuzzer/libinterfacespecification/specification/CameraHalV1.vts /data/local/tmp/spec/CameraHalV1.vts
adb push sysfuzzer/libinterfacespecification/specification/GpsHalV1.vts /data/local/tmp/spec/GpsHalV1.vts
adb push sysfuzzer/libinterfacespecification/specification/GpsHalV1GpsInterface.vts /data/local/tmp/spec/GpsHalV1GpsInterface.vts
adb push sysfuzzer/libinterfacespecification/specification/LightHalV1.vts /data/local/tmp/spec/LightHalV1.vts
adb push sysfuzzer/libinterfacespecification/specification/WifiHalV1.vts /data/local/tmp/spec/WifiHalV1.vts

# asan
adb push ../../prebuilts/clang/host/linux-x86/clang-2812033/lib64/clang/3.8/lib/linux/libclang_rt.asan-aarch64-android.so /data/local/tmp/libclang_rt.asan-aarch64-android.so

adb shell chmod 755 /data/local/tmp/fuzzer
adb shell chmod 755 /data/local/tmp/vts_hal_agent
adb shell killall vts_hal_agent > /dev/null 2&>1
adb shell killall fuzzer > /dev/null 2&>1
adb shell LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/vts_hal_agent /data/local/tmp/fuzzer /data/local/tmp/spec
# to run using nohup
# adb shell LD_LIBRARY_PATH=/data/local/tmp nohup /data/local/tmp/vts_hal_agent /data/local/tmp/fuzzer /data/local/tmp/spec
# ASAN_OPTIONS=coverage=1 for ASAN
