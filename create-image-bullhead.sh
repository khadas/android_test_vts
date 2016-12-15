#!/bin/bash

BASE_DIR=`pwd`/../..
echo $BASE_DIR

. ${BASE_DIR}/build/envsetup.sh
cd ${BASE_DIR}; lunch bullhead-userdebug
cd ${BASE_DIR}/test/vts; mma -j 32 && cd ${BASE_DIR}; make vts -j 32

mkdir -p ${BASE_DIR}/test/vts/images
mkdir -p ${BASE_DIR}/test/vts/images/bullhead
mkdir -p ${BASE_DIR}/test/vts/images/bullhead/hal
mkdir -p ${BASE_DIR}/test/vts/images/bullhead/hal64
cp ${BASE_DIR}/out/target/product/bullhead/system/bin/fuzzer test/vts/images/bullhead/ -f
cp ${BASE_DIR}/out/target/product/bullhead/system/bin/vts_hal_agent test/vts/images/bullhead/ -f
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/libvts_common.so test/vts/images/bullhead/libvts_common32.so -f
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/libvts_common.so test/vts/images/bullhead/libvts_common.so -f
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/libvts_datatype.so test/vts/images/bullhead/libvts_datatype32.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/libvts_datatype.so test/vts/images/bullhead/libvts_datatype.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/libvts_interfacespecification.so test/vts/images/bullhead/libvts_interfacespecification32.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/libvts_interfacespecification.so test/vts/images/bullhead/libvts_interfacespecification.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/libvts_measurement.so test/vts/images/bullhead/libvts_measurement32.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/libvts_measurement.so test/vts/images/bullhead/libvts_measurement.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/libvts_codecoverage.so test/vts/images/bullhead/libvts_codecoverage32.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/libvts_codecoverage.so test/vts/images/bullhead/libvts_codecoverage.so
# HAL
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/hw/lights.bullhead-vts.so test/vts/images/bullhead/hal/lights.bullhead-vts.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/hw/lights.bullhead-vts.so test/vts/images/bullhead/hal64/lights.bullhead-vts.so

cd ${BASE_DIR}/test/vts; ./setup-bullhead.sh
