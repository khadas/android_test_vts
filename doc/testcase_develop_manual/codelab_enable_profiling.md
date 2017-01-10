# Codelab - Enable Profiling for VTS HIDL HAL Tests

By enable profiling for your VTS HIDL HAL test, you are expected to get:

 * __trace files__ that record each HAL API call happened during the test
   execution with the passed argument values as well as the return values.

 * __performance profiling data__ of each API call which is also displayed in
   the VTS dashboard if the dashboard feature is used.

## 1. Add profiler library to VTS

To enable profiling for your HAL testing, we need to add the corresponding
profiler library in two places:

* [vts_test_lib_hidl_package_list.mk](../../tools/build/tasks/list/vts_test_lib_hidl_package_list.mk).

* [HidlHalTest.push](../../tools/vts-tradefed/res/push_groups/HidlHalTest.push).

The name of the profiling library follow the pattern as:
`package_name@version-interface-vts-profiler.so`.
For example, the library name for NFC HAL is `android.hardware.nfc@1.0-INfc-vts.profiler.so`.

## 2. Modify Your VTS Test Case

If you have not already,
[Codelab for Host-Driven Tests](codelab_host_driven_test.md)
gives an overview of how to write a VTS test case. This section assumes you have
completed that codelab and have at least one VTS test case (either host-driven or
target-side) which you would like to enable profiling.

### 2.1. Target-Side Tests

This subsection describes how to enable profiling for target-side tests.

* Copy an existing test directory

  `$ cd hardware/interfaces/<HAL Name>/<version>/vts/functional/vts/testcases/hal/<HAL Name>/hidl/`

  `$ cp target target_profiling -rf`

  Then rename the test name from <HAL Name>HidlTargetTest to <HAL Name>HidlTargetProfilingTest everywhere.

* Set `enable-profiling` flag

  Add `<option name="enable-profiling" value="true" />` to the corresponding
`AndroidTest.xml` file under the `target_profiling` directory.

  An [example AndroidTest.xml file](../../../../hardware/interfaces/vibrator/1.0/vts/functional/vts/testcases/hal/vibrator/hidl/target_profiling/AndroidTest.xml)  
looks like:

---
```
<configuration description="Config for VTS VIBRATOR HIDL HAL's target-side test cases">
    <target_preparer class="com.android.compatibility.common.tradefed.targetprep.VtsFilePusher">
        <option name="push-group" value="HidlHalTest.push" />
    </target_preparer>
    <target_preparer class="com.android.tradefed.targetprep.VtsPythonVirtualenvPreparer" />
    <test class="com.android.tradefed.testtype.VtsMultiDeviceTest">
        <option name="test-module-name" value="VibratorHidlTargetProfilingTest" />
        <option name="binary-test-sources" value="
            _32bit::DATA/nativetest/vibrator_hidl_hal_test/vibrator_hidl_hal_test,
            _64bit::DATA/nativetest64/vibrator_hidl_hal_test/vibrator_hidl_hal_test,
            "/>
        <option name="binary-test-type" value="gtest" />
        <option name="test-timeout" value="1m" />
        <option name="enable-profiling" value="true" />
    </test>
</configuration>
```
---

* Schedule the profiling test

  Add the following line to [vts-serving-staging-hal-hidl-profiling.xml](../../tools/vts-tradefed/res/config/vts-serving-staging-hal-hidl-profiling.xml):

  `<option name="compatibility:include-filter" value="<HAL Name>HidlTargetProfilingTest" />`
  where `<HAL NAME>` is the name of your HAL.

* Subscribe the notification alert emails

  Please check (notification page)[../web/notification_samples.md] for the detailed instructions.

  Basically, now it is all set so let's wait for a day or so and then visit your VTS Dashboard.
  At that time, you should be able to add `<HAL Name>HidlTargetProfilingTest` to your favorite list.
  That is all you need to do in order to subscribe alert emails which will sent if any notably performance degradations are found by your profiling tests.
  Also if you click `<HAL Name>HidlTargetProfilingTest` in the dashboard main page,
  the test result page shows up where the top-left side shows the list of APIs which have some measured performance data.

### 2.2. Host-Driven Tests

This subsection describes how to enable profiling for host-driven tests.

* Copy an existing test directory

  `$ cd hardware/interfaces/<HAL Name>/<version>/vts/functional/vts/testcases/hal/<HAL Name>/hidl/`

  `$ cp host host_profiling -rf`

  Then rename the test name from <HAL Name>HidlHostTest to <HAL Name>HidlHostProfilingTest everywhere.

* Update the configs

  First, add `<option name="enable-profiling" value="true" />` to the corresponding
  `AndroidTest.xml` file (similar as target-side tests).

  Second, add the following code to the `setUpClass` function in your test script

```
if self.enable_profiling:
    profiling_utils.EnableVTSProfiling(self.dut.shell.one)
```

   Also, add the following code to the `tearDownClass` function in your test script

```
if self.enable_profiling:
    profiling_trace_path = getattr(
        self, self.VTS_PROFILING_TRACING_PATH, "")
    self.ProcessAndUploadTraceData(self.dut, profiling_trace_path)
    profiling_utils.DisableVTSProfiling(self.dut.shell.one)
```
