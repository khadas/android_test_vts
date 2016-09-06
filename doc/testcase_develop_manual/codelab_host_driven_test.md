# Codelab - VTS Host-Driven, Structural Test

## 1. Project setup

### 1.1. Setup a local Android repository

`$ export branch=master`  # currently, internal master is mainly supported.

`$ mkdir ${branch}`

`$ cd ${branch}`

`$ repo init -b ${branch} -u persistent-https://googleplex-android.git.corp.google.com/platform/manifest`

`$ repo sync -j 32`

Then to check the vts project directory,

`$ ls test/vts`

### 1.2. Create a test project

Create your test project directory under,

`$ mkdir test/vts/testcases/codelab/hello_world`  # note that the dir is already created.

In practice, use

- `test/vts/testcases/**host**/<your project name>` if your project is for HAL (Hardware Abstraction Layer) or libraries

- `test/vts/testcases/**kernel**/<your project name>` if your project is for kernel or kernel modules

### 1.3. Create Android.mk file

`$ vi test/vts/testcases/host/codelab/hello_world/Android.mk`

Then edit its contents to:

---
```makefile
LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)

include $(CLEAR_VARS)

LOCAL_MODULE := CodeLabHelloWorld
LOCAL_MODULE_CLASS := FAKE
LOCAL_IS_HOST_MODULE := true
LOCAL_COMPATIBILITY_SUITE := vts

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE):
  @echo "VTS host-driven test target: $(LOCAL_MODULE)"
  $(hide) touch $@

VTS_CONFIG_SRC_DIR := testcases/codelab/hello_world
include test/vts/tools/build/Android.host_config.mk
```
---

That file tells the Android build system the `CodeLabHelloWorld` VTS test case
so that the Android build system can store the compiled executable to
a designated output directory (under `out/` directory) for packaging.

### 1.4. Create AndroidTest.xml file

`$ vi test/vts/testcases/host/codelab/hello_world/AndroidTest.xml`

Then edit its contents to:

---
```xml
<configuration description="Config for VTS CodeLab HelloWorld test case">
    <target_preparer class="com.android.compatibility.common.tradefed.targetprep.VtsFilePusher">
        <option name="push-group" value="HostDrivenTest.push" />
    </target_preparer>
    <target_preparer class="com.android.tradefed.targetprep.VtsPythonVirtualenvPreparer">
    </target_preparer>
    <test class="com.android.tradefed.testtype.VtsMultiDeviceTest">
        <option name="test-case-path" value="vts/testcases/codelab/hello_world/CodeLabHelloWorldTest" />
        <option name="test-config-path" value="vts-config/testcases/codelab/hello_world/CodeLabHelloWorldTest.config" />
    </test>
</configuration>
```
---

## 2. Design your test case

### 2.1. Declare a python module

`$ touch test/vts/testcases/host/codelab/hello_world/__init__.py`

### 2.2. Actual test case code

`$ vi test/vts/testcases/host/codelab/hello_world/CodeLabHelloWorldTest.py`

Then edit its contents to:

---
```python
import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class CodeLabHelloWorldTest(base_test.BaseTestClass):
    """Two hello world test cases which use the shell driver."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]

    def testEcho1(self):
        """A simple testcase which sends a command."""
        self.dut.shell.InvokeTerminal("my_shell1")  # creates a remote shell instance.
        results = self.dut.shell.my_shell1.Execute("echo hello_world")  # runs a shell command.
        logging.info(str(results[const.STDOUT]))  # prints the stdout
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello_world")  # checks the stdout
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)  # checks the exit code

    def testEcho2(self):
        """A simple testcase which sends two commands."""
        self.dut.shell.InvokeTerminal("my_shell2")
        my_shell = getattr(self.dut.shell, "my_shell2")
        results = my_shell.Execute(["echo hello", "echo world"])
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 2)  # check the number of processed commands
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello")
        asserts.assertEqual(results[const.STDOUT][1].strip(), "world")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)
        asserts.assertEqual(results[const.EXIT_CODE][1], 0)


if __name__ == "__main__":
    test_runner.main()
```
---

### 2.3. Test case config

`$ vi test/vts/testcases/host/codelab/hello_world/CodeLabHelloWorldTest.config`

Then edit its contents to:

---
```json
{
    "test_bed":
    [
        {
            "name": "VTS-CodeLab-HelloWorld",
            "AndroidDevice": "*"
        }
    ],
    "log_path": "/tmp/logs",
    "test_paths": ["./"]
}
```
---

### 2.4. Create a test suite

`$ vi test/vts/tools/vts-tradefed/res/config/vts-codelab.xml`

Then edit its contents to:

---
```json
<configuration description="Run VTS CodeLab Tests">
    <include name="everything" />
    <option name="compatibility:plan" value="vts" />
    <option name="compatibility:include-filter" value="CodeLabHelloWorldTest" />
    <template-include name="reporters" default="basic-reporters" />
</configuration>
```
---


## 3. Build and Run

### 3.1. Build

`$ . build/envsetup.sh`

or `$ source ./build/envsetup.sh`

To run the tests against physical devices,

`$ lunch <your target> (e.g., bullhead-userdebug, angler-userdebug)`

`$ make vts -j 10`

### 3.2. Run

`$ vts-tradefed`

`> run vts-codelab`

If your test case can violate some SELinux rules, please run:

`host$ adb shell`

`target$ su`

`target$ setenforce 0`

## 4. Serving

[Dashboard](https://android-vts-internal.googleplex.com)

Once a new test case is added to one of the launched test suites,
it is automatically executed in a test lab (e.g., using some common devices).
The exact schedule, and the used branches and devices are all customizable.
Please contact an EngProd representative to your team, or vts-dev@google.com.
