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

import logging
from google.cloud import pubsub
from google.cloud.exceptions import NotFound
from oauth2client.service_account import ServiceAccountCredentials
from time import sleep


class CloudClient(object):
    """Communicates with App Engine to receive and run VTS tests.

    Represents the data in .gcno and .gcda files.

    Attributes:
        clientName The (string) name of the runner machine. This must be pre-
                   enrolled with the PubSub service.
        POLL_INTERVAL The fequency at which pubsub service is polled (seconds)
        MAX_MESSAGES The maximum number of commands to receive at once
    """

    POLL_INTERVAL = 5
    MAX_MESSAGES = 100

    def __init__(self, clientName, oauth2_service_json):
        """Inits the object with the client name and a PubSub subscription

        Args:
            clientName: the name of the client. Must be pre-enrolled with the
                        PubSub service.
            oauth2_service_json: path (string) to the service account JSON
                                 keyfile.
        """
        self.clientName = clientName
        credentials = ServiceAccountCredentials.from_json_keyfile_name(
            oauth2_service_json)
        self._client = pubsub.Client(credentials=credentials)
        self._topic = self._client.topic(clientName)
        self._sub = self._topic.subscription(clientName)

    def Pull(self):
        """Fetches new messages from the PubSub subscription.

        Receives and acknowledges the commands published to the client's
        subscription.

        Returns:
            list of commands (strings) from PubSub subscription.
        """
        logging.info("Waiting for commands: %s", self.clientName)
        results = self._sub.pull(return_immediately=True,
                                 max_messages=self.MAX_MESSAGES)

        if results:
            logging.info("Commands received: %s", results)
            self._sub.acknowledge([ack_id for ack_id, message in results])
            return [message.data for ack_id, message in results]

        return None

    def Run(self):
        """Indefinitely pulls and invokes new commands from the PubSub service.
        """
        try:
            while True:
                commands = self.Pull()
                #TODO @ryancampbell: execute the commands in tradefed
                print(commands)
                if not commands:
                    sleep(self.POLL_INTERVAL)
        except NotFound as e:
            logging.error("No subscription created for client %s",
                          self.clientName)

