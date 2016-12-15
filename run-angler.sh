#!/bin/bash

PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.light.conventional.SampleLightTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.bluetooth.conventional.SampleBluetoothTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz.hal_light.conventional.LightFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/fuzz/hal_light/conventional/LightFuzzTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz.hal_light.conventional_standalone.StandaloneLightFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/fuzz/hal_light/conventional_standalone/StandaloneLightFuzzTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.camera.conventional.SampleCameraTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.nfc.hidl.NfcHidlBasicTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.shell.SampleShellTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz_test.lib_bionic.LibBionicLibmFuzzTest
