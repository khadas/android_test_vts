# Codelab - VTS Native Code Coverage Measurement

## 1. Build Rule Configuration

### 1.1. Compile with code coverage instrumentation

To enable code global coverage instrumentation, first let's set
the let's set the environment. Copy the command to the terminal:

`export NATIVE_COVERAGE=true`

Next, copy and paste the following:

`LOCAL_NATIVE_COVERAGE := true`

to a target module specified in its Android.mk file.

An alternative way is to add:

```
LOCAL_SANITIZE := never
LOCAL_CLANG := true
LOCAL_CFLAGS += -fprofile-arcs -ftest-coverage
LOCAL_LDFLAGS += --coverage
```

directly to the same file.

### 1.2. Copy Source and GCNO files

For the build system to be able to copy the target source and GCNO files, the
following shall be added right below a target module specified its Android.mk
file.

```
include $(BUILD_SHARED_LIBRARY)
include test/vts/tools/build/Android.packaging_sharedlib.mk

VTS_GCOV_SRC_DIR := test/vts/hals/<module>/<product>
VTS_GCOV_SRC_CPP_FILES := $(LOCAL_SRC_FILES)
include test/vts/tools/build/Android.packaging_gcno.mk
```

An example for lights HAL is:

```
include $(BUILD_SHARED_LIBRARY)
include test/vts/tools/build/Android.packaging_sharedlib.mk

VTS_GCOV_SRC_DIR := test/vts/hals/light/bullhead
VTS_GCOV_SRC_CPP_FILES := $(LOCAL_SRC_FILES)
VTS_GCNO_OBJ_DIR_NAME := out/target/product/$(TARGET_PRODUCT)/obj
include test/vts/tools/build/Android.packaging_gcno.mk
```

and is available at `test/vts/hals/light/bullhead/Android.mk`.

## 2. Modify Your VTS Test Case

If you have not already,
[Codelab for Host-Driven Tests](codelab_host_driven_test.md)
gives an overview of how to write a VTS test case. This section assumes you have
completed that codelab and have at least one VTS test case which you would like
to enable this code coverage measurement.

### 2.1. Enable uploading

In order to upload the measured code coverage data to a Google App Engine (GAE)
hosted database and show that on your VTS dashboard, let's add:

`"use_gae_db": true`

to `<target test case name>.config` file.

Then let's also specify the source files which you have enabled code coverage
instrumentation and would like to see the measured line coverage data by adding

`"coverage_src_files": ["<module_name>/<source file name with extension>"]`

to the same config file.

Then the target config file would look like:

```
{
    "test_bed":
    ...
    "log_path": "/tmp/logs",
    "test_paths": ["./"],
    "use_gae_db": true,
    "coverage_src_files": ["lights.bullhead-vts/lights.c"]]
}
```

### 2.2. Test case code

In each test case (i.e., a method whose name starts with `test`), let's add
the following right after each API call.

```
result = self.dut.hal.<target hal module name>.<target api>(<a list of args>)
last_coverage_data = {}
for coverage_msg in result.raw_coverage_data:
    last_coverage_data[coverage_msg.file_path] = coverage_msg.gcda
```

At the end of each test case, let's also add:

`self.SetCoverageData(last_coverage_data)`

so that the VTS runner knows about the measured coverage data.

An example is available at
`test/vts/testcases/fuzz/hal_light/conventional_standalone/StandaloneLightFuzzTest.py`.

## 3. Serving

That's all. Once all your changes are merged, let's wait for a couple of hours
until the test serving infrastructure catches up. Then visit your VTS web site
to find the measured code coverage result.
