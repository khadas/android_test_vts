vts_test_host_bin_packages := \
    host_cross_vndk-vtable-dumper \
    mkdtimg \
    trace_processor \
    vndk-vtable-dumper \
    img2simg \
    simg2img \
    mkuserimg_mke2fs.sh \

# TODO(hridya): b/72697311 Ensure that the test works on Windows OS
vts_test_host_bin_packages += \
    ufdt_verify_overlay_host \
    verifyDTBO.sh \
