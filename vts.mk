# mk file for a device to package all the VTS packages.

PRODUCT_PACKAGES += \
    vtssysfuzzer \
    vtsc \
    libvts_interfacespecification \
    vts_hal_agent \
    lights.bullhead-vts \
    camera.bullhead-vts

-include test/vts/sysfuzzer/libinterfacespecification/vts_specification.mk
