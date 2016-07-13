#!/bin/bash

PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.light.conventional.SampleLightTest $ANDROID_BUILD_TOP/test/vts/testcases/host/light/conventional/SampleLightTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.light.conventional_fuzz.SampleLightFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/host/light/conventional_fuzz/SampleLightFuzzTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.camera.conventional.SampleCameraTest $ANDROID_BUILD_TOP/test/vts/testcases/host/camera/conventional/SampleCameraTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.nfc.hidl.NfcHidlBasicTest $ANDROID_BUILD_TOP/test/vts/testcases/host/nfc/hidl/NfcHidlBasicTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.shell.SampleShellTest $ANDROID_BUILD_TOP/test/vts/testcases/host/shell/SampleShellTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz_test.lib_bionic.LibBionicLibmFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/fuzz_test/lib_bionic/LibBionicLibmFuzzTest.config
