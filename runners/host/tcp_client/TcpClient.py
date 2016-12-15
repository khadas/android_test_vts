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
#

import json
import logging
import os
import socket

from vts.runners.host.proto import AndroidSystemControlMessage_pb2

TARGET_IP = os.environ.get("TARGET_IP", None)
TARGET_PORT = os.environ.get("TARGET_PORT", 5001)
_SOCKET_CONN_TIMEOUT_SECS = 60
COMMAND_TYPE_NAME = {1: "Check binder service",
                     2: "Start binder service",
                     101: "Get HALs",
                     102: "Select a HAL",
                     201: "Get functions",
                     202: "Call function"}


class VtsTcpClient(object):
    """VTS TCP Client class.

  Attribute:
    connection: a TCP socket instance.
    channel: a file to write and read data.
    _mode: the connection mode (adb_forwarding or ssh_tunnel)
  """

    def __init__(self, mode="adb_forwarding"):
        self.connection = None
        self.channel = None
        self._mode = mode

    def Connect(self, ip=TARGET_IP, port=TARGET_PORT):
        """Connects to a target device.

    Args:
      ip: string, the IP adress of a target device.
      port: integer, the TCP port.

    Raises:
      Exception when the connection fails.
    """
        try:
            if self._mode == "adb_forwarding":
                os.system("adb forward --remove tcp:%s" % port)
                os.system("adb forward tcp:%s tcp:%s" % (port, port))
                ip = "localhost"
                logging.info("Connecting to %s:%s", ip, port)
                self.connection = socket.create_connection(
                    (ip, port), _SOCKET_CONN_TIMEOUT_SECS)
            elif self._mode == "ssh_tunnel":
                logging.info("Connecting to %s:%s", ip, port)
                self.connection = socket.create_connection(
                    (ip, port), _SOCKET_CONN_TIMEOUT_SECS)
        except socket.error as e:
            logging.exception(e)
            # TODO: use a custom exception
            raise
        self.channel = self.connection.makefile(mode="brw")

    def Disconnect(self):
        """Disconnects from the target device."""
        if self.connection is not None:
            self.channel = None
            self.connection.close()
            self.connection = None

    def SendCommand(self,
                    command_type,
                    target_name,
                    target_class=None,
                    target_type=None,
                    target_version=None):
        """Sends a command.

    Args:
      command_type: integer, the command type.
      target_name: string, the target name.
    """
        if not self.channel:
            Connect()

        command_msg = AndroidSystemControlMessage_pb2.AndroidSystemControlCommandMessage(
        )
        command_msg.command_type = command_type
        logging.info("sending a command (type %s)",
                     COMMAND_TYPE_NAME[command_type])
        command_msg.target_name = target_name

        if target_class is not None:
            command_msg.target_class = target_class

        if target_type is not None:
            command_msg.target_type = target_type

        if target_version is not None:
            command_msg.target_version = int(target_version * 100)

        message = command_msg.SerializeToString()
        message_len = len(message)
        logging.debug("sending %d bytes", message_len)
        self.channel.write(str(message_len) + b'\n')
        self.channel.write(message)
        self.channel.flush()

    def RecvResponse(self):
        """Receives and parses the response, and returns the relevant ResponseMessage."""
        try:
            header = self.channel.readline()
            len = int(header.strip("\n"))
            data = self.channel.read(len)
            response_msg = AndroidSystemControlMessage_pb2.AndroidSystemControlResponseMessage(
            )
            response_msg.ParseFromString(data)
            logging.info("Response %s", "succcess"
                         if response_msg.response_code ==
                         AndroidSystemControlMessage_pb2.SUCCESS else "fail")
            return response_msg
        except socket.timeout as e:
            logging.exception(e)
            return None
