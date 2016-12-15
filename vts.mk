# mk file for a device to package all the VTS packages.

ifneq ($(filter vts treble, $(TARGET_DEVICE)),)

PRODUCT_PACKAGES += \
    vtssysfuzzer \
    vtsc \
    libvts_interfacespecification \
    vts_hal_agent \
    libvts_profiling \
    lights.bullhead-vts \

-include test/vts/specification/vts_specification.mk

endif
