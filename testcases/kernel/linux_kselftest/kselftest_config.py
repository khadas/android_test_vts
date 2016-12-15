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

from vts.testcases.kernel.linux_kselftest import test_case

class ConfigKeys(object):
    RUN_STAGING = "run_staging"

class ExitCode(object):
    """Exit codes for test binaries and test scripts."""
    KSFT_PASS = 0
    KSFT_FAIL = 1
    KSFT_XPASS = 2
    KSFT_XFAIL = 3
    KSFT_SKIP = 4

# Directory on the target where the tests are copied.
KSFT_DIR = "/data/local/tmp/linux-kselftest"

KSFT_CASES_STABLE = map(lambda x: test_case.LinuxKselftestTestcase(*(x)), [
    ("futex/functional/futex_wait_timeout", ["arm", "x86"], [32, 64]),
    ("futex/functional/futex_wait_wouldblock", ["arm", "x86"], [32, 64]),
    ("futex/functional/futex_requeue_pi_mismatched_ops", ["arm", "x86"], [32, 64]),
    ("futex/functional/futex_wait_uninitialized_heap", ["arm", "x86"], [32]),
    ("futex/functional/futex_wait_private_mapped_file", ["arm", "x86"], [32, 64]),
    ("net/socket", ["arm", "x86"], [32, 64]),
])

KSFT_CASES_STAGING = map(lambda x: test_case.LinuxKselftestTestcase(*(x)), [
    ("net/psock_tpacket", ["arm", "x86"], [32, 64]),
    ("pstore/pstore_tests", ["arm", "x86"], [32, 64]),
    ("ptrace/peeksiginfo", ["arm", "x86"], [64]),
    ("seccomp/seccomp_bpf", ["arm", "x86"], [32, 64]),
    ("timers/posix_timers", ["arm", "x86"], [32, 64]),
    ("timers/nanosleep", ["arm", "x86"], [32, 64]),
    ("timers/nsleep-lat", ["arm", "x86"], [32, 64]),
    ("timers/set-timer-lat", ["arm", "x86"], [32, 64]),
    ("timers/inconsistency-check", ["arm", "x86"], [32, 64]),
    ("timers/alarmtimer-suspend", ["arm", "x86"], [32, 64]),
    ("timers/raw_skew", ["arm", "x86"], [32, 64]),
    ("timers/threadtest", ["arm", "x86"], [32, 64]),
    ("timers/change_skew", ["arm", "x86"], [64]),
    ("timers/skew_consistency", ["arm", "x86"], [64]),
    ("timers/clocksource-switch", ["arm", "x86"], [64]),
    ("timers/set-tai", ["arm", "x86"], [32, 64]),
    ("timers/valid-adjtimex", ["arm", "x86"], [64]),
])
