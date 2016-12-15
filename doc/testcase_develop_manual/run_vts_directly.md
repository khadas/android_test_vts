# Run a VTS test directly

First of all, if you have not done [VTS setup](../setup/index.md), that is required here.

## Build Binaries

`$ cd test/vts`

`$ ./create-image-<your build target>.sh`

For angler_treble and bullhead, please run:

`$ ./create-image-angler_treble.sh`

and

`$ ./create-image-bullhead.sh`

respectively.

## Copy Binaries

`$ ./setup-<your build target>.sh`

For angler_treble and bullhead, please run:

`$ ./setup-angler_treble.sh`

and

`$ ./setup-bullhead.sh`

respectively.

## Run a test direclty

`$ PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.<test py package path> $ANDROID_BUILD_TOP/test/vts/testcases/<path to its config file>`

For example, for SampleShellTest, please run:

`PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.shell.SampleShellTest $ANDROID_BUILD_TOP/test/vts/testcases/host/shell/SampleShellTest.config`

## Additional Step for LTP and Linux-Kselftest

Add `'data_file_path' : '<your android build top>/out/host/linux-x86/vts/android-vts/testcases'`
to your config file (e.g., `KernelLtpStagingTest.config`).

## Add a new test

In order to add a new test, the following two files needed to be extended.

`test/vts/create-image.sh`

`test/vts/setup.sh`

Optionally, the command used to add a new test can be also added to:

`test/vts/run-angler.sh`
