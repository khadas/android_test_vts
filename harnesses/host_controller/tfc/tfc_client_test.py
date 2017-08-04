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

from vts.harnesses.host_controller.tfc import tfc_client
from vts.harnesses.host_controller.tfc import device_info
from vts.harnesses.host_controller.tfc import request


class TfcClientTest(unittest.TestCase):
    """A test for tfc_client.TfcClient.

    Attributes:
        _client: The tfc_client.TfcClient being tested.
        _service: The mock service that _client connects to.
    """
    _DEVICE_INFOS = [device_info.DeviceInfo(
            device_serial="ABCDEF", group_name="group0",
            run_target="sailfish", state="Available")]

    def setUp(self):
        """Creates a TFC client connecting to a mock service."""
        self._service = mock.Mock()
        self._client = tfc_client.TfcClient(self._service)

    def testNewRequest(self):
        """Tests requests.new."""
        req = request.Request(cluster="cluster0",
                              command_line="vts-codelab",
                              run_target="sailfish",
                              user="user0")
        self._client.NewRequest(req)
        self._service.assert_has_calls([
                mock.call.requests().new().execute()])

    def testLeaseHostTasks(self):
        """Tests tasks.leasehosttasks."""
        self._client.LeaseHostTasks("cluster0", ["cluster1", "cluster2"],
                                    "host0", self._DEVICE_INFOS)
        self._service.assert_has_calls([
                mock.call.tasks().leasehosttasks().execute()])

    def testHostEvents(self):
        """Tests host_events.submit."""
        device_snapshots = [
                self._client.CreateDeviceSnapshot("vts-staging", "host0",
                                                  self._DEVICE_INFOS),
                self._client.CreateDeviceSnapshot("vts-presubmit", "host0",
                                                  self._DEVICE_INFOS)]
        self._client.SubmitHostEvents(device_snapshots)
        self._service.assert_has_calls([
                mock.call.host_events().submit().execute()])

    def testWrongParameter(self):
        """Tests raising exception for wrong parameter name."""
        self.assertRaisesRegexp(KeyError, "sdk", device_info.DeviceInfo,
                                device_serial="123", sdk="25")


if __name__ == "__main__":
    unittest.main()
