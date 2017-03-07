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
from vts.proto import VtsReportMessage_pb2 as ReportMsg
from vts.runners.host import asserts
from vts.runners.host import const
from vts.runners.host import keys
from vts.utils.python.web import feature_utils

LOCAL_PROFILING_TRACE_PATH = "/tmp/vts-test-trace"
TARGET_PROFILING_TRACE_PATH = "/data/local/tmp/"
HAL_INSTRUMENTATION_LIB_PATH = "/data/local/tmp/64/"

_PROFILING_DATA = "profiling_data"
_HOST_PROFILING_DATA = "host_profiling_data"

class VTSProfilingData(object):
    """Class to store the VTS profiling data.

    Attributes:
        values: A dict that stores the profiling data. e.g. latencies of each api.
        options: A set of strings where each string specifies an associated
                 option (which is the form of 'key=value').
    """

    def __init__(self):
        self.values = {}
        self.options = set()


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


class ProfilingFeature(feature_utils.Feature):
    """Feature object for profiling functionality.

    Attributes:
        enabled: boolean, True if profiling is enabled, False otherwise
        web: (optional) WebFeature, object storing web feature util for test run
    """

    _TOGGLE_PARAM = keys.ConfigKeys.IKEY_ENABLE_PROFILING
    _OPTIONAL_PARAMS = [keys.ConfigKeys.IKEY_PROFILING_TRACING_PATH]

    def __init__(self, user_params, web=None):
        """Initializes the profiling feature.

        Args:
            user_params: A dictionary from parameter name (String) to parameter value.
            web: (optional) WebFeature, object storing web feature util for test run
        """
        self.ParseParameters(self._TOGGLE_PARAM, optional_param_names=self._OPTIONAL_PARAMS,
                             user_params=user_params)
        self.web = web
        logging.info("Profiling enabled: %s", self.enabled)

    @staticmethod
    def _IsEventFromBinderizedHal(event_type):
        """Returns True if the event type is from a binderized HAL."""
        if event_type in [8, 9]:
            return False
        return True

    @staticmethod
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

    @staticmethod
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

    @staticmethod
    def DisableVTSProfiling(shell):
        """ Disable profiling by resetting the system property.

        Args:
            shell: shell to control the testing device.
        """
        shell.Execute("setprop hal.instrumentation.lib.path \"\"")
        shell.Execute("setprop hal.instrumentation.enable false")

    @classmethod
    def _ParseTraceData(cls, trace_file):
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

                if cls._IsEventFromBinderizedHal(vts_profiling_record.event):
                    profiling_data.options.add("hidl_hal_mode=binder")
                else:
                    profiling_data.options.add("hidl_hal_mode=passthrough")

                api = vts_profiling_record.func_msg.name
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
            profiling_data.values[api] = latencies
        return profiling_data

    def StartHostProfiling(self, name):
        """Starts a profiling operation.

        Args:
            name: string, the name of a profiling point

        Returns:
            True if successful, False otherwise
        """
        if not self.enabled:
            return False

        if not hasattr(self, _HOST_PROFILING_DATA):
            setattr(self, _HOST_PROFILING_DATA, {})

        host_profiling_data = getattr(self, _HOST_PROFILING_DATA)

        if name in host_profiling_data:
            logging.error("profiling point %s is already active.", name)
            return False
        host_profiling_data[name] = feature_utils.GetTimestamp()
        return True

    def StopHostProfiling(self, name):
        """Stops a profiling operation.

        Args:
            name: string, the name of a profiling point
        """
        if not self.enabled:
            return

        if not hasattr(self, _HOST_PROFILING_DATA):
            setattr(self, _HOST_PROFILING_DATA, {})

        host_profiling_data = getattr(self, _HOST_PROFILING_DATA)

        if name not in host_profiling_data:
            logging.error("profiling point %s is not active.", name)
            return False

        start_timestamp = host_profiling_data[name]
        end_timestamp = feature_utils.GetTimestamp()
        if self.web and self.web.enabled:
            self.web.AddProfilingDataTimestamp(
                name,
                start_timestamp,
                end_timestamp)
        return True

    def ProcessTraceDataForTestCase(self, dut):
        """Pulls the generated trace file to the host, parses the trace file to
        get the profiling data (e.g. latency of each API call) and stores these
        data in _profiling_data.

        Requires the feature to be enabled; no-op otherwise.

        Args:
            dut: the registered device.
        """
        if not self.enabled:
            return

        if not hasattr(self, _PROFILING_DATA):
            setattr(self, _PROFILING_DATA, [])

        profiling_data = getattr(self, _PROFILING_DATA)

        trace_files = self.GetTraceFiles(
            dut, getattr(self, keys.ConfigKeys.IKEY_PROFILING_TRACING_PATH))
        for file in trace_files:
            logging.info("parsing trace file: %s.", file)
            data = self._ParseTraceData(file)
            profiling_data.append(data)

    def ProcessAndUploadTraceData(self):
        """Process and upload profiling trace data.

        Requires the feature to be enabled; no-op otherwise.

        Merges the profiling data generated by each test case, calculates the
        aggregated max/min/avg latency for each API and uploads these latency
        metrics to webdb.
        """
        if not self.enabled:
            return

        merged_profiling_data = VTSProfilingData()
        for data in getattr(self, _PROFILING_DATA, []):
            for item in data.options:
                merged_profiling_data.options.add(item)
            for api, latences in data.values.items():
                if merged_profiling_data.values.get(api):
                    merged_profiling_data.values[api].extend(latences)
                else:
                    merged_profiling_data.values[api] = latences
        for api, latencies in merged_profiling_data.values.items():
            if not self.web or not self.web.enabled:
                continue

            self.web.AddProfilingDataUnlabeledVector(
                api,
                latencies,
                merged_profiling_data.options,
                x_axis_label="API processing latency (nano secs)",
                y_axis_label="Frequency")
