#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import sys

from vts.runners.host import base_test
from vts.runners.host.logger import Log
from vts.utils.python.mirror_objects import Mirror


class SampleCameraTestcase(base_test.BaseTestClass):
    """A sample testcase for the non-HIDL, conventional Camera HAL."""

    def testCamera(self):
        """A simple testcase which just calls a function."""
        logging.info("testCamera start")
        hal_mirror = Mirror.Mirror(["/data/local/tmp/32/hal"])
        hal_mirror.InitHal("camera", 2.1, bits=32)
        hal_mirror.camera.get_number_of_cameras()
        logging.info("testCamera end")


def main(args):
    """Main function which calls the framework's Main()."""
    # TODO: call base_test.Main(args) instead.
    Log.SetupLogger()
    # TODO: use the test runner instead.
    testcase = SampleCameraTestcase({})
    testcase.testCamera()


if __name__ == "__main__":
    main(sys.argv[1:])
