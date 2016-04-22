# mk file for a device to package all the VTS packages.

PRODUCT_PACKAGES += \
    vtssysfuzzer \
    vtsc \
    libvts_interfacespecification \

-include test/vts/sysfuzzer/libinterfacespecification/vts_specification.mk
