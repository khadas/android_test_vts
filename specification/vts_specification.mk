# mk file to copy VTS specs as part of packaging
#
PRODUCT_COPY_FILES += \
    test/vts/specification/hal_conventional/CameraHalV2.vts:system/etc/CameraHalV2.vts \
    test/vts/specification/hal_conventional/CameraHalV2hw_device_t.vts:system/etc/CameraHalV2hw_device_t.vts \
    test/vts/specification/hal_conventional/CameraHalV3.vts:system/etc/CameraHalV3.vts \
    test/vts/specification/hal_conventional/CameraHalV3camera3_device_ops_t.vts:system/etc/CameraHalV3camera3_device_ops_t.vts \
    test/vts/specification/hal_conventional/GpsHalV1.vts:system/etc/GpsHalV1.vts \
    test/vts/specification/hal_conventional/GpsHalV1GpsInterface.vts:system/etc/GpsHalV1GpsInterface.vts \
    test/vts/specification/hal_conventional/LightHalV1.vts:system/etc/LightHalV1.vts \
    test/vts/specification/hal_conventional/WifiHalV1.vts:system/etc/WifiHalV1.vts \
    test/vts/specification/hal_conventional/BluetoothHalV1.vts:system/etc/BluetoothHalV1.vts \
    test/vts/specification/hal_conventional/BluetoothHalV1bt_interface_t.vts:system/etc/BluetoothHalV1bt_interface_t.vts \
    test/vts/specification/hal_hidl/Nfc.vts:system/etc/Nfc.vts \
    test/vts/specification/hal_hidl/NfcClientCallback.vts:system/etc/NfcClientCallback.vts \
    test/vts/specification/lib_bionic/libmV1.vts:system/etc/libmV1.vts \
    test/vts/specification/lib_bionic/libcV1.vts:system/etc/libcV1.vts \
