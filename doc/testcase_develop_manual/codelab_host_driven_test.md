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

Create your test project using:

`test/vts/script/create-test-project.py --name <your project name> --dir <your project directory>`

Project name is required to be UpperCamel case (trailing numbers are allowed), and test directory
is a relative directory under `test/vts/testcases`.

For example, to create your test project `HelloAndroid` under directory
`test/vts/testcases/codelab/hello_android`, use the following command:

`test/vts/script/create-test-project.py --name HelloAndroid --dir codelab/hello_android`

This will create `Android.mk file`, `AndroidTest.xml`, `__init__.py`, and populate parent directories
with necessary packaging files.

The actual python test case file, which contains two simple hello world tests, is created under the
specified project directory using the lower under score version of the project name.

In practice, use

- `-dir host/<your_project_name>` if your project is for HAL (Hardware Abstraction Layer) or libraries

- `-dir kernel/<your_project_name>` if your project is for kernel or kernel modules

### 1.3. Create a test suite

`$ vi test/vts/tools/vts-tradefed/res/config/vts-codelab.xml`

Then edit its contents to:

---
```json
<configuration description="Run VTS CodeLab Tests">
    <include name="everything" />
    <option name="compatibility:plan" value="vts" />
    <option name="compatibility:include-filter" value="<your project name>" />
    <template-include name="reporters" default="basic-reporters" />
</configuration>
```
---

Multiple instances of `compatibility:include-filter` option can be added to include more tests under a test suite.


## 2. Build and Run

### 2.1. Build

`$ . build/envsetup.sh`

or `$ source ./build/envsetup.sh`

To run the tests against physical devices,

`$ lunch <your target> (e.g., bullhead-userdebug, angler-userdebug)`

`$ make vts -j 10`

### 2.2. Run

`$ vts-tradefed`

`> run vts-codelab`

If your test case can violate some SELinux rules, please run:

`host$ adb shell`

`target$ su`

`target$ setenforce 0`


## 3. Customize your test configuration (Optional)

### 3.1. AndroidTest.xml file

Pre-test file pushes from host to device can be configured for `VtsFilePusher` in `AndroidTest.xml`.

By default, `AndroidText.xml` pushes a group of files required to run VTS framework specified in
`test/vts/tools/vts-tradefed/res/push_groups/HidlHalTest.push`. Individual file push can be defined with
"push" option inside `VtsFilePusher`. Please refer to TradeFed for more detail.

Python module dependencies can be specified as "dep-module" option for
`VtsPythonVirtualenvPreparer` in `Android.xml`. This will trigger the runner to install or update
the modules using pip before running tests. `VtsPythonVirtualenvPreparer` will install a set of
package including future, futures, enum, and protobuf by default. To add a dependency module,
please add `<option name="dep-module" value="<module name>" />` inside `VtsPythonVirtualenvPreparer`
 in 'AndroidTest.xml'

### 3.2. Test case config

Optionally, a .config file can be used to pass variables in json format to test case.

To add a .config file, create a .config file under your project directory using project name:

`$ vi test/vts/testcases/host/<your project directiry>/<your project name>.config`

Then edit its contents to:

---
```json
{
    <key_1>: <value_1>,
    <key_2>: <value_2>
}
```
---

And in your test case python class, you can get the json value by using self.getUserParams method.

For example:

---
```py
    required_params = ["key_1"]
    self.getUserParams(required_params)
    logging.info("%s: %s", "key_1", self.key_1)
```
---

At last, add the following line to `com.android.tradefed.testtype.VtsMultiDeviceTest` class
in `AndroidTest.xml`:

`<option name="test-config-path" value="vts-config/testcases/<your project directiry>/<your project name>.config" />`

Your config file will overwrite the following default json object defined at
`test/vts/tools/vts-tradefed/res/default/DefaultTestCase.config`

---
```json
{
    "test_bed":
    [
        {
            "name": "<your project name>",
            "AndroidDevice": "*"
        }
    ],
    "log_path": "/tmp/logs",
    "test_paths": ["./"]
}
```
---


## 4. Serving

[Dashboard](https://android-vts-internal.googleplex.com)

Once a new test case is added to one of the launched test suites,
it is automatically executed in a test lab (e.g., using some common devices).
The exact schedule, and the used branches and devices are all customizable.
Please contact an EngProd representative to your team, or vts-dev@google.com.
