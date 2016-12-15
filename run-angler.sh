#!/bin/bash

# PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.sample.SampleLightFuzzTest $ANDROID_BUILD_TOP/test/vts/testcases/host/sample/SampleLightFuzzTest.config
PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.sample.SampleCameraTest $ANDROID_BUILD_TOP/test/vts/testcases/host/sample/SampleCameraTest.config
