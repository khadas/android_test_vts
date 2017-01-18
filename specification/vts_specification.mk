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
    hardware/interfaces/graphics/allocator/2.0/vts/Allocator.vts:system/etc/Allocator.vts \
    hardware/interfaces/graphics/allocator/2.0/vts/AllocatorClient.vts:system/etc/AllocatorClient.vts \
    hardware/interfaces/graphics/allocator/2.0/vts/types.vts:system/etc/types.vts \
    hardware/interfaces/graphics/mapper/2.0/vts/Allocator.vts:system/etc/Mapper.vts \
    hardware/interfaces/graphics/mapper/2.0/vts/types.vts:system/etc/types.vts \
    hardware/interfaces/nfc/1.0/vts/Nfc.vts:system/etc/Nfc.vts \
    hardware/interfaces/nfc/1.0/vts/NfcClientCallback.vts:system/etc/NfcClientCallback.vts \
    hardware/interfaces/nfc/1.0/vts/types.vts:system/etc/types.vts \
    hardware/interfaces/vehicle/2.0/vts/Vehicle.vts:system/etc/Vehicle.vts \
    hardware/interfaces/vehicle/2.0/vts/VehicleCallback.vts:system/etc/VehicleCallback.vts \
    hardware/interfaces/vehicle/2.0/vts/types.vts:system/etc/types.vts \
    hardware/interfaces/vibrator/1.0/vts/Vibrator.vts:system/etc/Vibrator.vts \
    hardware/interfaces/vibrator/1.0/vts/types.vts:system/etc/types.vts \
    hardware/interfaces/thermal/1.0/vts/Thermal.vts:system/etc/Thermal.vts \
    hardware/interfaces/thermal/1.0/vts/types.vts:system/etc/types.vts \
    hardware/interfaces/vr/1.0/vts/Vr.vts:system/etc/Vr.vts \
    hardware/interfaces/tv/cec/1.0/vts/HdmiCec.vts:system/etc/HdmiCec.vts \
    hardware/interfaces/tv/cec/1.0/vts/HdmiCecCallback.vts:system/etc/HdmiCecCallback.vts \
    hardware/interfaces/tv/cec/1.0/vts/types.vts:system/etc/types.vts \
    hardware/interfaces/tv/input/1.0/vts/TvInput.vts:system/etc/TvInput.vts \
    hardware/interfaces/tv/input/1.0/vts/TvInputCallback.vts:system/etc/TvInputCallback.vts \
    hardware/interfaces/tv/input/1.0/vts/types.vts:system/etc/types.vts \
    test/vts/specification/lib_bionic/libmV1.vts:system/etc/libmV1.vts \
    test/vts/specification/lib_bionic/libcV1.vts:system/etc/libcV1.vts \
