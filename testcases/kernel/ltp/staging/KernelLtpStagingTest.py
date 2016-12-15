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

from vts.runners.host import test_runner
from vts.testcases.kernel.ltp import KernelLtpTest


class KernelLtpStagingTest(KernelLtpTest.KernelLtpTest):
    """KernelLtpTest class' staging version.

    Since our database uses class name as identifier at this moment,
    this class is created to just use a different name from KernelLtpTest for
    result reporting purpose.

    """
    pass

if __name__ == "__main__":
    test_runner.main()