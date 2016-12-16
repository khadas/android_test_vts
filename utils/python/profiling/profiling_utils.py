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

from google.protobuf import text_format
from vts.proto import VtsProfilingMessage_pb2 as VtsProfilingMsg
from vts.runners.host import asserts
from vts.runners.host import const

LOCAL_PROFILING_TRACE_PATH = "/tmp/vts-test-trace"
TARGET_PROFILING_TRACE_PATH = "/data/local/tmp/"
HAL_INSTRUMENTATION_LIB_PATH = "/data/local/tmp/64/hw/"


def EnableVTSProfiling(
        shell, hal_instrumentation_lib_path=HAL_INSTRUMENTATION_LIB_PATH):
    """ Enable profiling by setting the system property.

    Args:
        shell: shell to control the testing device.
        hal_instrumentation_lib_path: directory that stores profiling libraries.
    """
    # cleanup any existing traces.
    shell.Execute("rm " + os.path.join(TARGET_PROFILING_TRACE_PATH,
                                       "*.vts.trace"))
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


class VTSProfilingData(object):
    """Class to store the VTS profiling data.

    Attributes:
        name: A string to describe the profiling data. e.g. server_side_latency.
        labels: A list of profiling data labels. e.g. a list of api names.
        values: A dict that stores the profiling data for different metrics.
    """

    def __init__(self):
        self.name = ""
        self.labels = []
        self.values = {"avg": [], "max": [], "min": []}


EVENT_TYPE_DICT = {
    0: "SERVER_API_ENTRY",
    1: "SERVER_API_EXIT",
    2: "CLIENT_API_ENTRY",
    3: "CLIENT_API_EXIT",
    4: "SYNC_CALLBACK_ENTRY",
    5: "SYNC_CALLBACK_EXIT",
    6: "ASYNC_CALLBACK_ENTRY",
    7: "ASYNC_CALLBACK_EXIT",
    8: "PASSTHROUGH_ENTRY",
    9: "PASSTHROUGH_EXIT",
}


def ParseTraceData(trace_file):
    """Parses the data stored in trace_file, calculates the avg/max/min
    latency for each API.

    Args:
        trace_file: file that stores the trace data.

    Returns:
        VTSProfilingData which contain the list of API names and the avg/max/min
        latency for each API.
    """
    profiling_data = VTSProfilingData()
    api_timestamps = {}
    api_latencies = {}

    myfile = open(trace_file, "r")
    new_entry = True
    profiling_record_str = ""
    for line in myfile.readlines():
        if not line.strip():
            new_entry = False
        if new_entry:
            profiling_record_str += line
        else:
            vts_profiling_record = VtsProfilingMsg.VtsProfilingRecord()
            text_format.Merge(profiling_record_str, vts_profiling_record)
            if not profiling_data.name:
                logging.warning("no name set for the profiling data. ")
                # TODO(zhuoyao): figure out a better way to set the data name.
                profiling_data.name = EVENT_TYPE_DICT[
                    vts_profiling_record.event]
            api = vts_profiling_record.interface
            timestamp = vts_profiling_record.timestamp
            if api_timestamps.get(api):
                api_timestamps[api].append(timestamp)
            else:
                api_timestamps[api] = [timestamp]
            new_entry = True
    for api, time_stamps in api_timestamps.items():
        latencies = []
        # TODO(zhuoyao): figure out a way to get the latencies, e.g based on the
        # event type of each entry.
        for index in range(1, len(time_stamps), 2):
            latencies.append(
                long(time_stamps[index]) - long(time_stamps[index - 1]))
        api_latencies[api] = latencies
    for api, latencies in api_latencies.items():
        profiling_data.labels.append(api)
        profiling_data.values["max"].append(max(latencies))
        profiling_data.values["min"].append(min(latencies))
        profiling_data.values["avg"].append(sum(latencies) / len(latencies))

    return profiling_data


def GetTraceFiles(dut, host_profiling_trace_path):
    """Pulls the trace file and save it under the profiling trace path.

    Args:
        dut: the testing device.
        host_profiling_trace_path: directory that stores trace files on host.

    Returns:
        Name list of trace files that stored on host.
    """
    if not host_profiling_trace_path:
        host_profiling_trace_path = LOCAL_PROFILING_TRACE_PATH
    if not os.path.exists(host_profiling_trace_path):
        os.makedirs(host_profiling_trace_path)
    logging.info("Saving profiling traces under: %s",
                 host_profiling_trace_path)

    dut.shell.InvokeTerminal("profiling_shell")
    results = dut.shell.profiling_shell.Execute("ls " + os.path.join(
        TARGET_PROFILING_TRACE_PATH, "*.vts.trace"))
    asserts.assertTrue(results, "failed to find trace file")
    stdout_lines = results[const.STDOUT][0].split("\n")
    logging.info("stdout: %s", stdout_lines)
    trace_files = []
    for line in stdout_lines:
        if line:
            file_name = os.path.join(host_profiling_trace_path,
                                     os.path.basename(line.strip()))
            dut.adb.pull("%s %s" % (line, file_name))
            trace_files.append(file_name)
    return trace_files
