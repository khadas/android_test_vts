LOCAL_PATH := $(call my-dir)

test_cflags = \
    -fstack-protector-all \
    -g \
    -Wall -Wextra -Wunused \
    -Werror \
    -fno-builtin \

test_cflags += -D__STDC_LIMIT_MACROS  # For glibc.

test_cppflags := \

test_executable := VtsHalLightsTestCases
list_executable := $(test_executable)_list

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_MODULE := $(test_executable)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/nativetest
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE)32
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE)64

LOCAL_SHARED_LIBRARIES += \
    libdl \
    libhardware \

LOCAL_WHOLE_STATIC_LIBRARIES += \
    libHalLightsTests \
    libVtsGtestMain \

LOCAL_STATIC_LIBRARIES += \
    libbase \
    libtinyxml2 \
    liblog \
    libgtest \

# Tag this module as a vts test artifact
LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_EXECUTABLE)

ifeq ($(HOST_OS)-$(HOST_ARCH),$(filter $(HOST_OS)-$(HOST_ARCH),linux-x86 linux-x86_64))
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := $(list_executable)
LOCAL_MULTILIB := both
# Use the 32 bit list executable since it will include some 32 bit only tests.
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE)32
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE)64

LOCAL_LDLIBS += \
    -lrt -ldl -lutil \

LOCAL_WHOLE_STATIC_LIBRARIES += \
    libHalLightsTests \
    libVtsGtestMain \

LOCAL_STATIC_LIBRARIES += \
    libbase \
    liblog \
    libcutils \

LOCAL_CXX_STL := libc++

include $(BUILD_HOST_NATIVE_TEST)
endif  # ifeq ($(HOST_OS)-$(HOST_ARCH),$(filter $(HOST_OS)-$(HOST_ARCH),linux-x86 linux-x86_64))


# -----------------------------------------------------------------------------
# Unit tests.
# -----------------------------------------------------------------------------

ifeq ($(HOST_OS)-$(HOST_ARCH),$(filter $(HOST_OS)-$(HOST_ARCH),linux-x86 linux-x86_64))
build_host := true
else
build_host := false
endif

common_additional_dependencies := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/../Android.build.mk

# -----------------------------------------------------------------------------
# All standard tests.
# -----------------------------------------------------------------------------

libHalLightsStandardTests_src_files := \
    hal_lights_basic_test.cpp

libHalLightsStandardTests_cflags := \
    $(test_cflags) \

libHalLightsStandardTests_cppflags := \
    $(test_cppflags) \

libHalLightsStandardTests_c_includes := \
    test/vts/testcases/include \
    external/tinyxml2 \

libHalLightsStandardTests_static_libraries := \
    libbase \

libHalLightsStandardTests_ldlibs_host := \
    -lrt \

# Clang/llvm has incompatible long double (fp128) for x86_64.
# https://llvm.org/bugs/show_bug.cgi?id=23897
# This affects most of math_test.cpp.
ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH),x86_64))
libHalLightsStandardTests_clang_target := false
endif

module := libHalLightsStandardTests
module_tag := optional
build_type := target
build_target := STATIC_TEST_LIBRARY
include $(LOCAL_PATH)/../Android.build.mk
build_type := host
include $(LOCAL_PATH)/../Android.build.mk

# Library of all tests (excluding the dynamic linker tests).
# -----------------------------------------------------------------------------
libHalLightsTests_whole_static_libraries := \
    libHalLightsStandardTests \


module := libHalLightsTests
module_tag := optional
build_type := target
build_target := STATIC_TEST_LIBRARY
include $(LOCAL_PATH)/../Android.build.mk
build_type := host
include $(LOCAL_PATH)/../Android.build.mk

# -----------------------------------------------------------------------------
# Tests for the device using HAL Lights's .so. Run with:
#   adb shell /data/nativetest/hallights-unit-tests/hallights-unit-tests32
#   adb shell /data/nativetest/hallights-unit-tests/hallights-unit-tests64
#   adb shell /data/nativetest/hallights-unit-tests/hallights-unit-tests-gcc32
#   adb shell /data/nativetest/hallights-unit-tests/hallights-unit-tests-gcc64
# -----------------------------------------------------------------------------
common_hallights-unit-tests_whole_static_libraries := \
    libHalLightsTests \
    libVtsGtestMain \

common_hallights-unit-tests_static_libraries := \
    libtinyxml2 \
    liblog \
    libbase \

# TODO: Include __cxa_thread_atexit_test.cpp to glibc tests once it is upgraded (glibc 2.18+)
common_hallights-unit-tests_src_files := \
    hal_lights_basic_test.cpp \

common_hallights-unit-tests_cflags := $(test_cflags)

common_hallights-unit-tests_conlyflags := \
    -fexceptions \
    -fnon-call-exceptions \

common_hallights-unit-tests_cppflags := \
    $(test_cppflags)

common_hallights-unit-tests_ldflags := \
    -Wl,--export-dynamic

common_hallights-unit-tests_c_includes := \
    bionic/libc \

common_hallights-unit-tests_shared_libraries_target := \
    libdl \
    libhardware \
    libpagemap \
    libdl_preempt_test_1 \
    libdl_preempt_test_2 \
    libdl_test_df_1_global \

# The order of these libraries matters, do not shuffle them.
common_hallights-unit-tests_static_libraries_target := \
    libbase \
    libziparchive \
    libz \
    libutils \

module_tag := optional
build_type := target
build_target := NATIVE_TEST

module := hallights-unit-tests
hallights-unit-tests_clang_target := true
hallights-unit-tests_whole_static_libraries := $(common_hallights-unit-tests_whole_static_libraries)
hallights-unit-tests_static_libraries := $(common_hallights-unit-tests_static_libraries)
hallights-unit-tests_src_files := $(common_hallights-unit-tests_src_files)
hallights-unit-tests_cflags := $(common_hallights-unit-tests_cflags)
hallights-unit-tests_conlyflags := $(common_hallights-unit-tests_conlyflags)
hallights-unit-tests_cppflags := $(common_hallights-unit-tests_cppflags)
hallights-unit-tests_ldflags := $(common_hallights-unit-tests_ldflags)
hallights-unit-tests_c_includes := $(common_hallights-unit-tests_c_includes)
hallights-unit-tests_shared_libraries_target := $(common_hallights-unit-tests_shared_libraries_target)
hallights-unit-tests_static_libraries_target := $(common_hallights-unit-tests_static_libraries_target)
include $(LOCAL_PATH)/../Android.build.mk

module := hallights-unit-tests-gcc
hallights-unit-tests-gcc_clang_target := false
hallights-unit-tests-gcc_whole_static_libraries := $(common_hallights-unit-tests_whole_static_libraries)
hallights-unit-tests-gcc_static_libraries := $(common_hallights-unit-tests_static_libraries)
hallights-unit-tests-gcc_src_files := $(common_hallights-unit-tests_src_files)
hallights-unit-tests-gcc_cflags := $(common_hallights-unit-tests_cflags)
hallights-unit-tests-gcc_conlyflags := $(common_hallights-unit-tests_conlyflags)
hallights-unit-tests-gcc_cppflags := $(common_hallights-unit-tests_cppflags)
hallights-unit-tests-gcc_ldflags := $(common_hallights-unit-tests_ldflags)
hallights-unit-tests-gcc_c_includes := $(common_hallights-unit-tests_c_includes)
hallights-unit-tests-gcc_shared_libraries_target := $(common_hallights-unit-tests_shared_libraries_target)
hallights-unit-tests-gcc_static_libraries_target := $(common_hallights-unit-tests_static_libraries_target)
include $(LOCAL_PATH)/../Android.build.mk

# -----------------------------------------------------------------------------
# Tests for the device linked against hallights's static library. Run with:
#   adb shell /data/nativetest/hallights-unit-tests-static/hallights-unit-tests-static32
#   adb shell /data/nativetest/hallights-unit-tests-static/hallights-unit-tests-static64
# -----------------------------------------------------------------------------
hallights-unit-tests-static_whole_static_libraries := \
    libVtsGtestMain \

hallights-unit-tests-static_static_libraries := \
    libm \
    libc \
    libc++_static \
    libdl \
    libtinyxml2 \
    liblog \
    libbase \

hallights-unit-tests-static_force_static_executable := true

# libc and libc++ both define std::nothrow. libc's is a private symbol, but this
# still causes issues when linking libc.a and libc++.a, since private isn't
# effective until it has been linked. To fix this, just allow multiple symbol
# definitions for the static tests.
hallights-unit-tests-static_ldflags := -Wl,--allow-multiple-definition

module := hallights-unit-tests-static
module_tag := optional
build_type := target
build_target := NATIVE_TEST
include $(LOCAL_PATH)/../Android.build.mk

include $(call first-makefiles-under,$(LOCAL_PATH))
