#!/usr/bin/env python
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

import logging
import os

from vts.runners.host import asserts
from vts.runners.host import const

LOCAL_PROFILING_TRACE_PATH = "/tmp/vts-test-trace"
TARGET_PROFILING_TRACE_PATH = "/data/local/tmp/"
HAL_INSTRUMENTATION_LIB_PATH = "/data/local/tmp/64/hw/"


def EnableVTSProfiling(
        shell,
        target_profiling_trace_path=TARGET_PROFILING_TRACE_PATH,
        hal_instrumentation_lib_path=HAL_INSTRUMENTATION_LIB_PATH):
    """ Enable profiling by setting the system property.

    Args:
      shell: shell to control the testing device.
      target_profiling_trace_path: directory that stores trace files on target.
      hal_instrumentation_lib_path: directory that stores profiling libraries.
    """
    # cleanup any existing traces.
    shell.Execute("rm " + os.path.join(target_profiling_trace_path, "*.trace"))
    logging.info("enable VTS profiling.")

    shell.Execute("setprop hal.instrumentation.lib.path " +
                  hal_instrumentation_lib_path)
    shell.Execute("setprop hal.instrumentation.enable true")

def DisableVTSProfiling(shell):
    """ Disable profiling by resetting the system property.

    Args:
      shell: shell to control the testing device.
    """
    shell.Execute("setprop hal.instrumentation.lib.path \"\"")
    shell.Execute("setprop hal.instrumentation.enable false")


def GetTraceData(dut,
                 host_profiling_trace_path,
                 target_profiling_trace_path=TARGET_PROFILING_TRACE_PATH):
    """ Pull the trace file and save it under the profiling trace path.

    Args:
      dut: the testing device.
      host_profiling_trace_path: directory that stores trace files on host.
      target_profiling_trace_path: directory that stores trace files on target.
    """
    if not host_profiling_trace_path:
        host_profiling_trace_path = LOCAL_PROFILING_TRACE_PATH
    if not os.path.exists(host_profiling_trace_path):
        os.makedirs(host_profiling_trace_path)
    logging.info("Saving profiling traces under: %s", host_profiling_trace_path)

    dut.shell.InvokeTerminal("profiling_shell")
    results = dut.shell.profiling_shell.Execute("ls " + os.path.join(
        target_profiling_trace_path, "*.trace"))
    asserts.assertTrue(results, "failed to find trace file")
    stdout_lines = results[const.STDOUT][0].split("\n")
    logging.info("stdout: %s", stdout_lines)
    for line in stdout_lines:
        if (line):
            dut.adb.pull("%s %s" % (line, host_profiling_trace_path))
