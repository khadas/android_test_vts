#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

import unittest

try:
    from unittest import mock
except ImportError:
    import mock

try:
    import StringIO as string_io_module
except ImportError:
    import io as string_io_module

from vts.harnesses.host_controller.tfc import command_task
from vts.harnesses.host_controller.tfc import device_info
from vts.harnesses.host_controller import console


class ConsoleTest(unittest.TestCase):
    """A test for console.Console.

    Attribute:
        _in_file: The console input buffer.
        _out_file: The console output buffer.
        _host_controller: A mock host_controller.HostController.
        _tfc_client: A mock tfc_client.TfcClient.
        _console: The console being tested.
    """
    _DEVICES = [device_info.DeviceInfo(device_serial="ABC001",
                                      run_target="sailfish",
                                      state="Available",
                                      build_id="111111",
                                      sdk_version="27")]
    _TASKS = [command_task.CommandTask(request_id="1",
                                       task_id="1-0",
                                       command_id="2",
                                       command_line="vts -m SampleShellTest",
                                       device_serials=["ABC001"])]

    def setUp(self):
        """Creates the console."""
        self._in_file = string_io_module.StringIO()
        self._out_file = string_io_module.StringIO()
        self._host_controller = mock.Mock()
        self._tfc_client = mock.Mock()
        self._console = console.Console(self._tfc_client,
                                        [self._host_controller],
                                        self._in_file,
                                        self._out_file)

    def _IssueCommand(self, command_line):
        """Issues a command in the console.

        Args:
            command_line: A string, the input to the console.

        Returns:
            A string, the output of the console.
        """
        in_position = self._in_file.tell()
        self._in_file.write(command_line)
        self._in_file.seek(in_position)

        out_position = self._out_file.tell()
        self._console.onecmd(command_line)
        self._out_file.seek(out_position)
        return self._out_file.read()

    def testLease(self):
        """Tests the lease command."""
        self._host_controller.LeaseCommandTasks.return_value = self._TASKS
        output = self._IssueCommand("lease")
        expected = ("request_id  command_id  task_id  device_serials  command_line          \n"
                    "1           2           1-0      ABC001          vts -m SampleShellTest\n")
        self.assertEqual(expected, output)
        output = self._IssueCommand("lease --host 0")
        self.assertEqual(expected, output)

    def testRequest(self):
        """Tests the request command."""
        user = "user0"
        cluster = "cluster0"
        run_target = "sailfish"
        command_line = "vts -m SampleShellTest"
        self._IssueCommand("request --user %s --cluster %s --run-target %s "
                           "-- %s" % (user, cluster, run_target, command_line))
        req = self._tfc_client.NewRequest.call_args[0][0]
        self.assertEqual(user, req.user)
        self.assertEqual(cluster, req.cluster)
        self.assertEqual(run_target, req.run_target)
        self.assertEqual(command_line, req.command_line)

    def testListHosts(self):
        """Tests the list command."""
        self._host_controller.hostname = "host0"
        output = self._IssueCommand("list hosts")
        self.assertEqual("index  name\n"
                         "[  0]  host0\n",
                         output)

    def testListDevices(self):
        """Tests the list command."""
        self._host_controller.ListDevices.return_value = self._DEVICES
        self._host_controller.hostname = "host0"
        output = self._IssueCommand("list devices")
        expected = ("[  0]  host0\n"
                    "device_serial  state      run_target  build_id  sdk_version  stub\n"
                    "ABC001         Available  sailfish    111111    27               \n")
        self.assertEqual(expected, output)
        output = self._IssueCommand("list devices --host 0")
        self.assertEqual(expected, output)

    def testWrongHostIndex(self):
        """Tests host index out of range."""
        output = self._IssueCommand("list devices --host 1")
        expected = "IndexError: "
        self.assertTrue(output.startswith(expected))
        output = self._IssueCommand("lease --host 1")
        self.assertTrue(output.startswith(expected))


if __name__ == "__main__":
    unittest.main()
