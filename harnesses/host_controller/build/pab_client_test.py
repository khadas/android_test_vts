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
from vts.harnesses.host_controller.build import pab_client

try:
    from unittest import mock
except ImportError:
    import mock


class PartnerAndroidBuildClientTest(unittest.TestCase):
    """Tests for Partner Android Build client."""

    def testUrlFormat(self):
        expected_url = (
            "https://partnerdash.google.com/build/gmsdownload/"
            "f_companion/label/clockwork.companion_20170906_211311_RC00/"
            "ClockworkCompanionGoogleWithGmsRelease_signed.apk")
        client = pab_client.PartnerAndroidBuildClient()

        url = client.GetArtifactURL(
            'f_companion', 'label', 'clockwork.companion_20170906_211311_RC00',
            'ClockworkCompanionGoogleWithGmsRelease_signed.apk')
        self.assertEqual(url, expected_url)

    @mock.patch("pab_client.flow_from_clientsecrets")
    @mock.patch("pab_client.run_flow")
    @mock.patch("pab_client.Storage")
    @mock.patch('pab_client.PartnerAndroidBuildClient.credentials')
    def testAuthenticationNew(self, mock_creds, mock_storage, mock_rf,
                              mock_ffc):
        mock_creds.invalid = True
        client = pab_client.PartnerAndroidBuildClient()
        client.Authenticate()
        mock_ffc.assert_called_once()
        mock_storage.assert_called_once()
        mock_rf.assert_called_once()

    @mock.patch("pab_client.flow_from_clientsecrets")
    @mock.patch("pab_client.run_flow")
    @mock.patch("pab_client.Storage")
    @mock.patch('pab_client.PartnerAndroidBuildClient.credentials')
    def testAuthenticationStale(self, mock_creds, mock_storage, mock_rf,
                                mock_ffc):
        mock_creds.invalid = False
        mock_creds.access_token_expired = True
        client = pab_client.PartnerAndroidBuildClient()
        client.Authenticate()
        mock_ffc.assert_called_once()
        mock_storage.assert_called_once()
        mock_rf.assert_not_called()
        mock_creds.refresh.assert_called_once()

    @mock.patch("pab_client.flow_from_clientsecrets")
    @mock.patch("pab_client.run_flow")
    @mock.patch("pab_client.Storage")
    @mock.patch('pab_client.PartnerAndroidBuildClient.credentials')
    def testAuthenticationFresh(self, mock_creds, mock_storage, mock_rf,
                                mock_ffc):
        mock_creds.invalid = False
        mock_creds.access_token_expired = False
        client = pab_client.PartnerAndroidBuildClient()
        client.Authenticate()
        mock_ffc.assert_called_once()
        mock_storage.assert_called_once()
        mock_rf.assert_not_called()
        mock_creds.refresh.assert_not_called()

    @mock.patch('pab_client.PartnerAndroidBuildClient.credentials')
    @mock.patch('pab_client.requests')
    @mock.patch('pab_client.open')
    def testGetArtifact(self, mock_open, mock_requests, mock_creds):
        expected_url = (
            "https://partnerdash.google.com/build/gmsdownload/"
            "f_companion/label/clockwork.companion_20170906_211311_RC00/"
            "ClockworkCompanionGoogleWithGmsRelease_signed.apk")
        client = pab_client.PartnerAndroidBuildClient()
        client.GetArtifact(
            'f_companion', 'label', 'clockwork.companion_20170906_211311_RC00',
            'ClockworkCompanionGoogleWithGmsRelease_signed.apk', '100374304')
        mock_creds.apply.assert_called_with({})
        mock_requests.get.assert_called_with(
            expected_url, params={'a': '100374304'}, headers={}, stream=True)
        mock_open.assert_called_with(
            'ClockworkCompanionGoogleWithGmsRelease_signed.apk', 'wb')

        client.GetArtifact('f_companion', 'label',
                           'clockwork.companion_20170906_211311_RC00',
                           'ClockworkCompanionGoogleWithGmsRelease_signed.apk',
                           '100374304', 'NewFile.apk')
        mock_open.assert_called_with('NewFile.apk', 'wb')


if __name__ == "__main__":
    unittest.main()
