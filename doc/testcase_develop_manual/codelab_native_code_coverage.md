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

In most cases, no additional test configuration is needed to enable coverage.
By default, coverage processing is enabled on the target if it is
coverage-instrumented and if it has the property:

```
ro.vts.coverage=1
```
However, some customization is available in the test configuration JSON file.
If we would like to limit the modules for which coverage data is gathered, then
we can use the "modules" property. We must specify a list of dictionaries, each
containing the module name (i.e. the module name in the make file for the binary
or shared library), as well as git project name and path for the source code
included in the module. For example, add:

```
"modules": [{
                "module_name": "<module name>",
                "git_project": {
                                    "name": "<git project name>",
                                    "path": "<path to git project root>"
                               }
            },...]
```

to the config file. Note that the module name must match the local module
name specified in the Android.mk file for which coverage was enabled; necessary
coverage files are identified by their parent module. Also, project path should
be relative to the Android root.

For the lights HAL, the target config file would look like:

```
{
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

No changes are needed for target-side tests. Coverage will be automatic so long
as the test host harness inherits from vts.testcases.template.binary_test.BinaryTest
and the target is properly instrumented for coverage.

### 2.3. Host-Driven Tests

Host-driven tests have more flexibility for coverage measurement, as the host
may request coverage files after each API call, at the end of a test case, or when
all test cases have completed.

#### 2.3.1. Measure coverage at the end of an API call

Coverage is available with the result of each API call. To add it to the dashboard
for display, call `self.SetCoverageData` with the contents of `result.raw_coverage_data`.

For example, in a test of the lights HAL, the following would gather coverage
after an API call to set the light:

```
result = self.dut.hal.light.set_light(None, gene)   # host-driven API call to light HAL
self.coverage.SetCoverageData(result.raw_coverage_data)
```


#### 2.3.3. Measure coverage at the end of a test case

After a test case has completed, coverage can be gathered independently of an
API call. Coverage can be requested from the device under test (dut) with the
method `GetRawCodeCoverage()`.

For example, at the end of a host-side NFC test case, coverage data is fetched using:

```
self.coverage.SetCoverageData(self.dut.hal.nfc.GetRawCodeCoverage())
```


#### 2.3.4. Measure coverage by pulling all coverage files from the device

For coarse coverage measurement (e.g. after running all of the tests), coverage
can be requested by pulling any coverage-related output files from the device
manually over ADB. The base test class provides a coverage feature to fetch
and process the files.

In the case of NFC, the coverage data is fetched with the following code:

```
self.coverage.SetCoverageData(dut=self.dut, isGlobal=True)
```


## 3. Serving

That's all. Once all your changes are merged, let's wait for a couple of hours
until the test serving infrastructure catches up. Then visit your VTS web site
to find the measured code coverage result. Coverage information will be available
for tests run against coverage-instrumented device builds.
