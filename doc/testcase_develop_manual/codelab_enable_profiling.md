# Codelab - Enable Profiling for VTS HIDL HAL Tests

By enable profiling for your VTS HIDL HAL test, you are expected to get:

 * __trace files__ that record each HAL API call happened during the test
   execution with the passed argument values as well as the return values.

 * __performance profiling data__ of each API call which is also displayed in
   the VTS dashboard if the dashboard feature is used.

## 1. Add profiler library to VTS

To enable profiling for your HAL testing, we need to add the corresponding
profiler library in:

* [vts_test_lib_hidl_package_list.mk](../../tools/build/tasks/list/vts_test_lib_hidl_package_list.mk).

The name of the profiling library follow the pattern as:
`package_name@version-vts-profiler.so`.
For example, the library name for NFC HAL is `android.hardware.nfc@1.0-vts.profiler.so`.

## 2. Modify Your VTS Test Case

If you have not already,
[Codelab for Host-Driven Tests](codelab_host_driven_test.md)
gives an overview of how to write a VTS test case. This section assumes you have
completed that codelab and have at least one VTS test case (either host-driven or
target-side) which you would like to enable profiling.

### 2.1. Target-Side Tests

This subsection describes how to enable profiling for target-side tests.

* Copy an existing test directory

  `$ cd test/vts-testcase/hal/<HAL_NAME>/<HAL_VERSION>/`

  `$ cp target target_profiling -rf`

  where `<HAL_NAME>` is the name of your HAL and `<HAL_VERSION>` is the version of your HAL with format `V<MAJOR_VERSION>_<MINOR_VERSION>>` e.g. `V1_0`.
  Then rename the test name from VtsHal<HAL_NAME><HAL_VERSION>Target to VtsHal<HAL_NAME><HAL_VERSION>TargetProfiling everywhere.

* Set `enable-profiling` flag

  Add `<option name="enable-profiling" value="true" />` to the corresponding
`AndroidTest.xml` file under the `target_profiling` directory.

  An [example AndroidTest.xml file](../../../../test/vts-testcase/hal/vibrator/V1_0/target_profiling/AndroidTest.xml)
looks like:

---
```
<configuration description="Config for VTS VtsHalVibratorV1_0TargetProfiling test cases">
    <target_preparer class="com.android.compatibility.common.tradefed.targetprep.VtsFilePusher">
        <option name="push-group" value="HalHidlTargetProfilingTest.push" />
        <option name="cleanup" value="true"/>
        <option name="push" value="DATA/lib/android.hardware.vibrator@1.0-vts.profiler.so->/data/local/tmp/32/android.hardware.vibrator@1.0-vts.profiler.so"/>
        <option name="push" value="DATA/lib64/android.hardware.vibrator@1.0-vts.profiler.so->/data/local/tmp/64/android.hardware.vibrator@1.0-vts.profiler.so"/>
    </target_preparer>
    <target_preparer class="com.android.tradefed.targetprep.VtsPythonVirtualenvPreparer" />
    <test class="com.android.tradefed.testtype.VtsMultiDeviceTest">
        <option name="test-module-name" value="VtsHalVibratorV1_0TargetProfiling" />
        <option name="binary-test-source" value="_32bit::DATA/nativetest/VtsHalVibratorV1_0TargetTest/VtsHalVibratorV1_0TargetTest" />
        <option name="binary-test-source" value="_64bit::DATA/nativetest64/VtsHalVibratorV1_0TargetTest/VtsHalVibratorV1_0TargetTest" />
        <option name="binary-test-type" value="hal_hidl_gtest" />
        <option name="enable-profiling" value="true" />
        <option name="precondition-lshal" value="android.hardware.vibrator@1.0"/>
        <option name="test-timeout" value="1m" />
    </test>
</configuration>
```
---

* Schedule the profiling test

  Add the following line to [vts-serving-staging-hal-hidl-profiling.xml](../../tools/vts-tradefed/res/config/vts-serving-staging-hal-hidl-profiling.xml):

  `<option name="compatibility:include-filter" value="VtsHal<HAL_NAME><HAL_VERSION>TargetProfiling" />`

* Subscribe the notification alert emails

  Please check (notification page)[../web/notification_samples.md] for the detailed instructions.

  Basically, now it is all set so let's wait for a day or so and then visit your VTS Dashboard.
  At that time, you should be able to add `VtsHal<HAL_NAME><HAL_VERSION>TargetProfiling` to your favorite list.
  That is all you need to do in order to subscribe alert emails which will sent if any notably performance degradations are found by your profiling tests.
  Also if you click `VtsHal<HAL_NAME><HAL_VERSION>TargetProfiling` in the dashboard main page, the test result page shows up where the top-left side shows the list of APIs which have some measured performance data.

### 2.2. Host-Driven Tests

This subsection describes how to enable profiling for host-driven tests.

* Copy an existing test directory

  `$ cd test/vts-testcase/hal/<HAL_NAME>/<HAL_VERSION>/`

  `$ cp host host_profiling -rf`

  Then rename the test name from VtsHal<HAL_NAME><HAL_VERSION>Host to VtsHal<HAL_NAME><HAL_VERSION>HostProfiling everywhere.

* Update the configs

  First, add `<option name="enable-profiling" value="true" />` to the corresponding
  `AndroidTest.xml` file (similar as target-side tests).

  Second, add the following code to the `setUpClass` function in your test script

```
if self.profiling.enabled:
    self.profiling.EnableVTSProfiling(self.dut.shell.one)
```

   Also, add the following code to the `tearDownClass` function in your test script

```
if self.profiling.enabled:
    self.profiling.ProcessAndUploadTraceData(self.dut)
    self.profiling.DisableVTSProfiling(self.dut.shell.one)
```
