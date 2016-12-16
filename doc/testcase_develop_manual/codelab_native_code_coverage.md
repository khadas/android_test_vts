# Codelab - VTS Native Code Coverage Measurement

## 1. Enable Coverage Instrumentation on Device Build

To enable code global coverage instrumentation, two environment variables must be
set: NATIVE_COVERAGE must be set to true, and COVERAGE_PATHS must specify a comma-
separated list of directories for which coverage will be enabled. For example:

```
export NATIVE_COVERAGE=true
export COVERAGE_PATHS="test/vts/hals/light,hardware/interfaces"
```

## 2. Modify Your VTS Test Case

If you have not already,
[Codelab for Host-Driven Tests](codelab_host_driven_test.md)
gives an overview of how to write a VTS test case. This section assumes you have
completed that codelab and have at least one VTS test case (either host-driven or
target-side) which you would like to enable this code coverage measurement.

### 2.1. Configure the test

In order to upload the measured code coverage data to a Google App Engine (GAE)
hosted database and show that on your VTS dashboard, let's add:

`"use_gae_db": true` and `"coverage": true` to `<target test case name>.config` file.

Then let's also specify the source files which you have enabled code coverage
instrumentation and would like to see the measured line coverage data by adding:

```
"coverage": true,
"modules": [{
                "module_name": "<module name>",
                "git_project": {
                                    "name": "<git project name>",
                                    "path": "<path to git project root>"
                               }
            },...]
```

to the same config file. Note that the module name must match the local module
name specified in the Android.mk file for which coverage was enabled; necessary
coverage files are identified by their parent module. Also, project path should
be relative to the Android root.

For the lights HAL, the target config file would look like:

```
{
    "use_gae_db": true
    "coverage": true,
    "modules": [{
                    "module_name": "system/lib64/hw/lights.vts",
                    "git_project": {
                                        "name": "platform/test/vts",
                                        "path": "test/vts"
                                    }
                }]
}
```

### 2.2. Target-Side Tests

#### 2.2.1. Add build flags to the test build file

Locate the `Android.bp` file adjacent to the .cpp target-side test file. In order
for coverage files to be created at the end of test execution, the test file must
also be built with coverage flags. Add the flag `--coverage` to the cflags and
ldflags lists.

For example, the build file for the target-side NFC test has the following
definition:

```
cc_test {
    name: "nfc_hidl_hal_test",
    gtest: true,
    srcs: ["nfc_hidl_hal_test.cpp"],
    shared_libs: [
        "libbase",
        "liblog",
        "libcutils",
        "libhidlbase",
        "libhidltransport",
        "libhwbinder",
        "libnativehelper",
        "libutils",
        "android.hardware.nfc@1.0",
    ],
    static_libs: ["libgtest"],
    cflags: [
        "--coverage",
        "-O0",
        "-g",
    ],
    ldflags: [
        "--coverage"
    ]
}
```

The rest is automatic and coverage will automatically be uploaded to the VTS
dashboard with each invocation.

### 2.3. Host-Driven Tests

Host-driven tests have more flexibility for coverage measurement, as the host
may request coverage files after each API call, at the end of a test case, or when
all test cases have completed.

#### 2.3.1. Setup the test to run in passthrough mode.

Currently coverage for host-driven tests is only supported while the HAL is in
passthrough mode. To enable this, execute the following before initializing the
HAL:

```
self.dut.shell.one.Execute("setprop vts.hal.vts.hidl.get_stub true")
```

#### 2.3.2. Measure coverage at the end of an API call

Coverage is available with the result of each API call. To add it to the dashboard
for display, call `self.SetCoverageData` with the contents of `result.raw_coverage_data`.

For example, in a test of the lights HAL, the following would gather coverage
after an API call to set the light:

```
result = self.dut.hal.light.set_light(None, gene)   # host-driven API call to light HAL
self.SetCoverageData(result.raw_coverage_data)
```


#### 2.3.3. Measure coverage at the end of a test case

After a test case has completed, coverage can be gathered independently of an
API call. Coverage can be requested from the device under test (dut) with the
method `GetRawCodeCoverage()`.

For example, at the end of a host-side NFC test case, coverage data is fetched using:

```
self.SetCoverageData(self.dut.hal.nfc.GetRawCodeCoverage())
```

#### 2.3.4. Measure coverage by pulling all coverage files from the device

For coarse coverage measurement (e.g. after running all of the tests), coverage
can be requested by pulling any coverage-related output files from the device
manually over ADB. The `coverage_utils.py` utility provides a function to fetch
and process the files.

In the case of NFC, the coverage data is fetched with the following code:

```
from vts.utils.python.coverage import coverage_utils
self.SetCoverageData(coverage_utils.GetGcdaDict(self.dut.hal.nfc))
```


## 3. Serving

That's all. Once all your changes are merged, let's wait for a couple of hours
until the test serving infrastructure catches up. Then visit your VTS web site
to find the measured code coverage result. Coverage information will be available
for tests run against coverage-instrumented device builds.
