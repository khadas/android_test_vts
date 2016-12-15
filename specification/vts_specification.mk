# mk file to copy VTS specs as part of packaging
#
PRODUCT_COPY_FILES += \
    test/vts/specification/hal_conventional/CameraHalV2.vts:system/etc/CameraHalV2.vts \
    test/vts/specification/hal_conventional/CameraHalV2hw_device_t.vts:system/etc/CameraHalV2hw_device_t.vts \
    test/vts/specification/hal_conventional/GpsHalV1.vts:system/etc/GpsHalV1.vts \
    test/vts/specification/hal_conventional/GpsHalV1GpsInterface.vts:system/etc/GpsHalV1GpsInterface.vts \
    test/vts/specification/hal_conventional/LightHalV1.vts:system/etc/LightHalV1.vts \
    test/vts/specification/hal_conventional/WifiHalV1.vts:system/etc/WifiHalV1.vts \
