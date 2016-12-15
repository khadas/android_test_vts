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

CPT_HOTPLUG_TESTSUITE = "simpleperf_cpu_hotplug_test"

CPT_HOTPLUG_BASIC_TESTS = [
    "cpu_offline.offline_while_recording_on_another_cpu",
    "cpu_offline.offline_while_recording",
    "cpu_offline.offline_while_user_process_profiling",
]

# Tests that don't have a fix in kernel yet.
CPT_HOTPLUG_UNAVAILABLE_TESTS = [
    "cpu_offline.offline_while_ioctl_enable",
]

CPT_HOTPLUG_EXCLUDE_DEVICES = [
    # angler doesn't pass tests because of http://b/30971326.
    "angler",
]
