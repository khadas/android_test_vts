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

import threading
import unittest
import time

try:
    from unittest import mock
except ImportError:
    import mock

from vts.harnesses.host_controller import host_controller
from vts.harnesses.host_controller.tfc import command_task
from vts.harnesses.host_controller.tfc import device_info


class HostControllerTest(unittest.TestCase):
    """A test for host_controller.HostController.

    Args:
        _remote_client: A mock remote_client.RemoteClient.
        _tfc_client: A mock tfc_client.TfcClient.
        _host_controller: The HostController being tested.
    """
    _AVAILABLE_DEVICES = [
            device_info.DeviceInfo(device_serial="ABC001",
                                   run_target="sailfish",
                                   state="Available")]
    _UNAVAILABLE_DEVICES = [
            device_info.DeviceInfo(device_serial="ABC002",
                                   run_target="sailfish",
                                   state="Allocated"),
            device_info.DeviceInfo(device_serial="emulator-5554",
                                   run_target="unknown",
                                   state="Available",
                                   stub=True)]

    _TASKS = [command_task.CommandTask(task_id="1-0",
                                       command_line="vts -m SampleShellTest",
                                       device_serials=["ABC001"])]

    def setUp(self):
        """Creates the HostController."""
        self._remote_client = mock.Mock()
        self._tfc_client = mock.Mock()
        self._host_controller = host_controller.HostController(
                self._remote_client, self._tfc_client, "host1", ["cluster1"])

    @mock.patch("vts.harnesses.host_controller.invocation_thread."
                "InvocationThread.run")
    def testDeviceStateDuringInvocation(self, mock_run):
        """Tests LeaseHostTasks and ListAvailableDevices."""
        self._remote_client.ListDevices.return_value = (
                self._AVAILABLE_DEVICES + self._UNAVAILABLE_DEVICES)
        self._tfc_client.LeaseHostTasks.return_value = self._TASKS
        run_event = threading.Event()
        mock_run.side_effect = lambda: run_event.wait()

        self._host_controller.LeaseCommandTasks(self._AVAILABLE_DEVICES)
        devices = self._host_controller.ListAvailableDevices()
        self.assertEqual([], devices)
        run_event.set()
        # Wait for thread termination
        time.sleep(0.2)
        devices = self._host_controller.ListAvailableDevices()
        self.assertEqual(self._AVAILABLE_DEVICES, devices)


if __name__ == "__main__":
    unittest.main()
