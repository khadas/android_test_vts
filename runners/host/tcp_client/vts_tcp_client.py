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

from vts.runners.host.proto import AndroidSystemControlMessage_pb2 as SysMsg_pb2
from vts.runners.host.proto import InterfaceSpecificationMessage_pb2 as IfaceSpecMsg_pb2

from google.protobuf import text_format

TARGET_IP = os.environ.get("TARGET_IP", None)
TARGET_PORT = os.environ.get("TARGET_PORT", 5001)
_SOCKET_CONN_TIMEOUT_SECS = 60
COMMAND_TYPE_NAME = {1: "LIST_HALS",
                     2: "SET_HOST_INFO",
                     101: "CHECK_STUB_SERVICE",
                     102: "LAUNCH_STUB_SERVICE",
                     201: "LIST_APIS",
                     202: "CALL_API"}


class VtsTcpError(Exception):
    pass


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

    def Connect(self, ip=TARGET_IP, port=TARGET_PORT,
                callback_port=None):
        """Connects to a target device.

        Args:
            ip: string, the IP address of a target device.
            port: int, the TCP port which can be used to connect to
                  a target device.
            callback_port: int, the TCP port number of a host-side callback
                           server.

        Returns:
            True if success, False otherwise

        Raises:
            socket.error when the connection fails.
        """
        try:
            # TODO: This assumption is incorrect. Need to fix.
            if not ip:  # adb_forwarding
                logging.info("ADB port forwarding mode - connecting to tcp:%s",
                             port)
                ip = "localhost"
                self.connection = socket.create_connection(
                    (ip, port), _SOCKET_CONN_TIMEOUT_SECS)
            else:  # ssh_tunnel
                logging.info("SSH tunnel mode - connecting to %s:%s", ip, port)
                self.connection = socket.create_connection(
                    (ip, port), _SOCKET_CONN_TIMEOUT_SECS)
        except socket.error as e:
            logging.exception(e)
            # TODO: use a custom exception
            raise
        self.channel = self.connection.makefile(mode="brw")

        if callback_port is not None:
            self.SendCommand(SysMsg_pb2.SET_HOST_INFO,
                             callback_port=callback_port)
            resp = self.RecvResponse()
            if (resp.response_code != SysMsg_pb2.SUCCESS):
                return False
        return True

    def Disconnect(self):
        """Disconnects from the target device.

        TODO(yim): Send a msg to the target side to teardown handler session
        and release memory before closing the socket.
        """
        if self.connection is not None:
            self.channel = None
            self.connection.close()
            self.connection = None

    def ListHals(self, base_paths):
        """RPC to LIST_HALS."""
        self.SendCommand(SysMsg_pb2.LIST_HALS, paths=base_paths)
        resp = self.RecvResponse()
        if (resp.response_code == SysMsg_pb2.SUCCESS):
            return resp.file_names
        return None

    def CheckStubService(self, service_name):
        """RPC to CHECK_STUB_SERVICE."""
        self.SendCommand(SysMsg_pb2.CHECK_STUB_SERVICE,
                         service_name=service_name)
        resp = self.RecvResponse()
        return (resp.response_code == SysMsg_pb2.SUCCESS)

    def LaunchStubService(self, service_name, file_path, bits, target_class,
                          target_type, target_version):
        """RPC to LAUNCH_STUB_SERVICE."""
        logging.info("service_name: %s", service_name)
        logging.info("file_path: %s", file_path)
        logging.info("bits: %s", bits)
        self.SendCommand(SysMsg_pb2.LAUNCH_STUB_SERVICE,
                         service_name=service_name,
                         file_path=file_path,
                         bits=bits,
                         target_class=target_class,
                         target_type=target_type,
                         target_version=target_version)
        resp = self.RecvResponse()
        return (resp.response_code == SysMsg_pb2.SUCCESS)

    def ListApis(self):
        """RPC to LIST_APIS."""
        self.SendCommand(SysMsg_pb2.LIST_APIS)
        resp = self.RecvResponse()
        if (resp.response_code == SysMsg_pb2.SUCCESS):
            return resp.spec
        return None

    def CallApi(self, arg):
        """RPC to CALL_API."""
        self.SendCommand(SysMsg_pb2.CALL_API, arg=arg)
        resp = self.RecvResponse()
        resp_code = resp.response_code
        if (resp_code == SysMsg_pb2.SUCCESS):
            result = IfaceSpecMsg_pb2.FunctionSpecificationMessage()
            if resp.result == "error":
                raise VtsTcpError("API call error by the VTS stub.")
            try:
                text_format.Merge(resp.result, result)
            except text_format.ParseError as e:
                logging.error("Paring error\n%s\n%s", resp.result, e)
            return result
        logging.error("NOTICE - Likely a crash discovery!")
        logging.error("SysMsg_pb2.SUCCESS is %s", SysMsg_pb2.SUCCESS)
        raise VtsTcpError("RPC Error, response code for %s is %s" % (arg, resp_code))

    def SendCommand(self,
                    command_type,
                    paths=None,
                    file_path=None,
                    bits=None,
                    target_class=None,
                    target_type=None,
                    target_version=None,
                    module_name=None,
                    service_name=None,
                    callback_port=None,
                    arg=None):
        """Sends a command.

        Args:
            command_type: integer, the command type.
            each of the other args are to fill in a field in
            AndroidSystemControlCommandMessage.
        """
        if not self.channel:
            raise VtsTcpError("channel is None, unable to send command.")

        command_msg = SysMsg_pb2.AndroidSystemControlCommandMessage()
        command_msg.command_type = command_type
        logging.info("sending a command (type %s)",
                     COMMAND_TYPE_NAME[command_type])
        if command_type == 202:
            logging.info("target API: %s", arg)

        if target_class is not None:
            command_msg.target_class = target_class

        if target_type is not None:
            command_msg.target_type = target_type

        if target_version is not None:
            command_msg.target_version = int(target_version * 100)

        if module_name is not None:
            command_msg.module_name = module_name

        if service_name is not None:
            command_msg.service_name = service_name

        if paths is not None:
            command_msg.paths.extend(paths)

        if file_path is not None:
            command_msg.file_path = file_path

        if bits is not None:
            command_msg.bits = bits

        if callback_port is not None:
            command_msg.callback_port = callback_port

        if arg is not None:
            command_msg.arg = arg

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
            logging.info("resp %d bytes", len)
            data = self.channel.read(len)
            response_msg = SysMsg_pb2.AndroidSystemControlResponseMessage()
            response_msg.ParseFromString(data)
            logging.debug("Response %s", "success"
                          if response_msg.response_code == SysMsg_pb2.SUCCESS
                          else "fail")
            return response_msg
        except socket.timeout as e:
            logging.exception(e)
            return None
