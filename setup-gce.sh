#!/bin/bash

BASE_DIR=`pwd`/../..
echo $BASE_DIR

#adb root
adb push ${BASE_DIR}/out/target/product/gce_x86/system/bin/fuzzer /data/local/tmp/fuzzer
adb push ${BASE_DIR}/out/target/product/gce_x86/system/bin/vts_hal_agent /data/local/tmp/vts_hal_agent
adb shell chmod 755 /data/local/tmp/fuzzer
adb shell chmod 755 /data/local/tmp/vts_hal_agent
adb shell /data/local/tmp/vts_hal_agent
