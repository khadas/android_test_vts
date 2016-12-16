#!/bin/bash

PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.light.conventional.SampleLightTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.bluetooth.conventional.SampleBluetoothTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz.hal_light.conventional.LightFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/fuzz/hal_light/conventional/LightFuzzTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz.hal_light.conventional_standalone.StandaloneLightFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/fuzz/hal_light/conventional_standalone/StandaloneLightFuzzTest.config
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.camera.conventional.SampleCameraTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.camera.conventional.2_1.SampleCameraV2Test
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.camera.conventional.3_4.SampleCameraV3Test
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.nfc.hidl.host.NfcHidlBasicTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.hal.vibrator.hidl.VibratorHidlTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.shell.SampleShellTest
# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.fuzz_test.lib_bionic.LibBionicLibmFuzzTest
