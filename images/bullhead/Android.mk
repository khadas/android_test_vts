
VTS_TESTCASES_OUT := $(HOST_OUT)/vts/android-vts/testcases

vts_protobuf_lib32_file := $(VTS_TESTCASES_OUT)/libprotobuf-cpp-full.so
vts_protobuf_lib64_file := $(VTS_TESTCASES_OUT)/libprotobuf-cpp-full64.so

$(vts_protobuf_lib32_file): test/vts/images/bullhead/32/libprotobuf-cpp-full.so | $(ACP)
	$(hide) mkdir -p $(dir $@)
	$(hide) $(ACP) -fp $< $@

$(vts_protobuf_lib64_file): test/vts/images/bullhead/64/libprotobuf-cpp-full.so | $(ACP)
	$(hide) mkdir -p $(dir $@)
	$(hide) $(ACP) -fp $< $@

vts: $(vts_protobuf_lib32_file) $(vts_protobuf_lib32_file)

