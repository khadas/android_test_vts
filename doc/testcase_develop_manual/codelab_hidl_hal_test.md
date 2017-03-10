# Codelab - HIDL HAL Test

## 1. Setup

HIDL HAL testing often requires non-conventional test setup. VTS make it easy
for test developers to specific such setup requirements and procedures.

### 1.1. Precondition

Let's assume your test's AndroidTest.xml looks like following.

---
```xml
<test class="com.android.tradefed.testtype.VtsMultiDeviceTest">
    <option name="test-module-name" value="HalMyHidlTargetTest"/>
    <option name="binary-test-source" value="..." />
...
```
---

The following option is needed to use the HIDL HAL gtest template.

`<option name="binary-test-type" value="hal_hidl_gtest" />`

You can now use one of the following four preconditions to describe when your
HIDL HAL test should be run.

1. Option `precondition-hwbinder-service` is to specify
a hardware binder service needed to run the test.

 `<option name="precondition-hwbinder-service" value="android.hardware.my" />`

2. Option `precondition-feature` is to specify
the name of a `pm`-listable feature needed to run the test.

 `<option name="precondition-feature" value="android.hardware.my.high_performance" />`

3. Option `precondition-file-path-prefix` is to specify
the path prefix of a file (e.g., shared library) needed to run the test.

 `<option name="precondition-file-path-prefix" value="/*/lib*/hw/libmy." />`

4. Option `precondition-lshal` is to specify
the name of a `lshal`-listable feature needed to run the test.

 `<option name="precondition-lshal" value="android.hardware.my@1.0::IMy" />`

### 1.2. Other options

The option `skip-if-thermal-throttling` can be set to `true` if you want to
skip a test when your target device suffers from thermal throttling:

 `<option name="skip-if-thermal-throttling" value="true" />`

