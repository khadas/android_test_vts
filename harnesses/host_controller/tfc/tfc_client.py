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

import httplib2
import logging
import time

from apiclient import discovery
from oauth2client.service_account import ServiceAccountCredentials

API_NAME = "tradefed_cluster"
API_VERSION = "v1"
SCOPES = ['https://www.googleapis.com/auth/userinfo.email']


class TfcClient(object):
    """The class for accessing TFC API.

    Attributes:
        _service: The TFC service.
    """

    def __init__(self, service):
        self._service = service

    def LeaseHostTasks(self, cluster_id, next_cluster_ids, hostname, device_infos):
        """Calls leasehosttasks.

        Args:
            cluster_id: A string, the primary cluster to lease tasks from.
            next_cluster_ids: A list of Strings, the secondary clusters to lease
                              tasks from.
            hostname: A string, the name of the TradeFed host.
            device_infos: A list of DeviceInfo, the information about the
                          devices connected to the host.

        Returns:
            A JSON object, the leased tasks.

            Sample
            {'tasks': [{'request_id': '2',
                        'command_line': 'vts-codelab --serial ABCDEF',
                        'task_id': '1-0',
                        'device_serials': ['ABCDEF'],
                        'command_id': u'1'}]}
        """
        json_obj = {"hostname": hostname,
                    "cluster": cluster_id,
                    "next_cluster_ids": next_cluster_ids,
                    "device_infos": [x.ToLeaseHostTasksJson()
                                     for x in device_infos]}
        logging.info("tasks.leasehosttasks body=%s", json_obj)
        return self._service.tasks().leasehosttasks(body=json_obj).execute()

    @staticmethod
    def CreateDeviceSnapshot(cluster_id, hostname, dev_infos):
        """Creates a DeviceSnapshot which can be uploaded as host event.

        Args:
            cluster_id: A string, the cluster to upload snapshot to.
            hostname: A string, the name of the TradeFed host.
            dev_infos: A list of DeviceInfo.

        Returns:
            A JSON object.
        """
        obj = {"time": int(time.time()),
               "data": {},
               "cluster": cluster_id,
               "hostname": hostname,
               "tf_version": "(unknown)",
               "type": "DeviceSnapshot",
               "device_infos": [x.ToDeviceSnapshotJson() for x in dev_infos]}
        return obj

    def SubmitHostEvents(self, host_events):
        """Calls host_events.submit.

        Args:
            host_events: A list of JSON objects. Currently DeviceSnapshot is
                         the only type of host events.
        """
        json_obj = {"host_events": host_events}
        logging.info("host_events.submit body=%s", json_obj)
        self._service.host_events().submit(body=json_obj).execute()

    def NewRequest(self, request):
        """Calls requests.new.

        Args:
            request: An instance of Request.

        Returns:
            A JSON object, the new request queued in the cluster.

            Sample
            {'state': 'UNKNOWN',
             'command_line': 'vts-codelab --run-target sailfish',
             'id': '2',
             'user': 'testuser'}
        """
        body = request.GetBody()
        params = request.GetParameters()
        logging.info("requests.new parameters=%s body=%s", params, body)
        return self._service.requests().new(body=body, **params).execute()


def CreateTfcClient(api_root, oauth2_service_json):
    """Builds an object of TFC service from a given URL.

    Args:
        api_root: The URL to the service.
        oauth2_service_json: The path to service account key file.

    Returns:
        A TfcClient object.
    """
    discovery_url = "%s/discovery/v1/apis/%s/%s/rest" % (
            api_root, API_NAME, API_VERSION)
    logging.info("Build service from: %s", discovery_url)
    credentials = ServiceAccountCredentials.from_json_keyfile_name(
            oauth2_service_json, scopes=SCOPES)
    http = credentials.authorize(httplib2.Http())
    service = discovery.build(
            API_NAME, API_VERSION, http=http,
            discoveryServiceUrl=discovery_url)
    return TfcClient(service)
