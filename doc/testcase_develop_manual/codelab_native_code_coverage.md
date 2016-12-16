# Codelab - VTS Native Code Coverage Measurement

## 1. Build Rule Configuration

### 1.1. Enable code coverage instrumentation

To enable code global coverage instrumentation, two environment variables must be
set: NATIVE_COVERAGE must be set to true and COVERAGE_DIRS must specify a space-
delimited list of directories for which coverage will be enabled. For example:

```
export NATIVE_COVERAGE=true
export COVERAGE_DIRS="test/vts/hals/light"
```

To enable coverage on more than one directory, simply add it to the COVERAGE_DIRS
variable using spaces to delimit entries.




### 1.2. Configure module makefile

For the runner to be able to locate coverage files later, the
following must be added right below a target module specified its Android.mk
file.

```
include $(BUILD_SHARED_LIBRARY)
include test/vts/tools/build/Android.packaging_sharedlib.mk
```


## 2. Modify Your VTS Test Case

If you have not already,
[Codelab for Host-Driven Tests](codelab_host_driven_test.md)
gives an overview of how to write a VTS test case. This section assumes you have
completed that codelab and have at least one VTS test case which you would like
to enable this code coverage measurement.

### 2.1. Enable webservice uploading

In order to upload the measured code coverage data to a Google App Engine (GAE)
hosted database and show that on your VTS dashboard, let's add:

`"use_gae_db": true`

to `<target test case name>.config` file.

Then let's also specify the source files which you have enabled code coverage
instrumentation and would like to see the measured line coverage data by adding:

```
"modules": ["<module-name>",...],
"git_project_path": "<git-project-name>",
"git_project_name": "<git-project-path>"
```

to the same config file. Note that the module name must match the local module
name specified in the Android.mk file for which coverage was enabled; necessary
coverage files are identified by their parent module. Also, project path should
be relative to the Android root.

For the lights HAL, the target config file would look like:

```
{
    ...
    "modules": ["system/lib64/hw/lights.vts"],
    "git_project_path": "test/vts",
    "git_project_name": "platform/test/vts"
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
to find the measured code coverage result. Coverage information will be available
for tests run against coverage-instrumented device builds.
