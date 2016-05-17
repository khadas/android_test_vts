#!/bin/bash

BASE_DIR=`pwd`/../..
echo $BASE_DIR

. ${BASE_DIR}/build/envsetup.sh
cd ${BASE_DIR}; lunch bullhead-userdebug
cd ${BASE_DIR}; make -j 32 && make vts -j 32

mkdir -p images
mkdir -p images/bullhead
cp ${BASE_DIR}/out/target/product/bullhead/system/bin/fuzzer test/vts/images/bullhead/ -f
cp ${BASE_DIR}/out/target/product/bullhead/system/bin/vts_hal_agent test/vts/images/bullhead/ -f
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/libvts_common.so test/vts/images/bullhead/libvts_common32.so -f
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/libvts_common.so test/vts/images/bullhead/libvts_common.so -f
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/libvts_datatype.so test/vts/images/bullhead/libvts_datatype32.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/libvts_datatype.so test/vts/images/bullhead/libvts_datatype.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib/libvts_interfacespecification.so test/vts/images/bullhead/libvts_interfacespecification32.so
cp ${BASE_DIR}/out/target/product/bullhead/system/lib64/libvts_interfacespecification.so test/vts/images/bullhead/libvts_interfacespecification.so
