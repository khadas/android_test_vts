#!/bin/bash

BASE_DIR=`pwd`/../..
echo $BASE_DIR

. ${BASE_DIR}/build/envsetup.sh
cd ${BASE_DIR}; lunch angler-userdebug
cd ${BASE_DIR}/test/vts; mma -j 32 && cd ${BASE_DIR}; make vts -j 32

mkdir -p ${BASE_DIR}/test/vts/images
mkdir -p ${BASE_DIR}/test/vts/images/angler
mkdir -p ${BASE_DIR}/test/vts/images/angler/hal
mkdir -p ${BASE_DIR}/test/vts/images/angler/hal64
cp ${BASE_DIR}/out/target/product/angler/system/bin/fuzzer test/vts/images/angler/ -f
cp ${BASE_DIR}/out/target/product/angler/system/bin/vts_hal_agent test/vts/images/angler/ -f
cp ${BASE_DIR}/out/target/product/angler/system/lib/libvts_common.so test/vts/images/angler/libvts_common32.so -f
cp ${BASE_DIR}/out/target/product/angler/system/lib64/libvts_common.so test/vts/images/angler/libvts_common.so -f
cp ${BASE_DIR}/out/target/product/angler/system/lib/libvts_datatype.so test/vts/images/angler/libvts_datatype32.so
cp ${BASE_DIR}/out/target/product/angler/system/lib64/libvts_datatype.so test/vts/images/angler/libvts_datatype.so
cp ${BASE_DIR}/out/target/product/angler/system/lib/libvts_interfacespecification.so test/vts/images/angler/libvts_interfacespecification32.so
cp ${BASE_DIR}/out/target/product/angler/system/lib64/libvts_interfacespecification.so test/vts/images/angler/libvts_interfacespecification.so
cp ${BASE_DIR}/out/target/product/angler/system/lib/libvts_measurement.so test/vts/images/angler/libvts_measurement32.so
cp ${BASE_DIR}/out/target/product/angler/system/lib64/libvts_measurement.so test/vts/images/angler/libvts_measurement.so
cp ${BASE_DIR}/out/target/product/angler/system/lib/libvts_codecoverage.so test/vts/images/angler/libvts_codecoverage32.so
cp ${BASE_DIR}/out/target/product/angler/system/lib64/libvts_codecoverage.so test/vts/images/angler/libvts_codecoverage.so
# HAL
cp ${BASE_DIR}/out/target/product/angler/system/lib/hw/lights.bullhead-vts.so test/vts/images/angler/hal/lights.bullhead-vts.so
cp ${BASE_DIR}/out/target/product/angler/system/lib64/hw/lights.bullhead-vts.so test/vts/images/angler/hal64/lights.bullhead-vts.so

cd ${BASE_DIR}/test/vts; ./setup-angler.sh
