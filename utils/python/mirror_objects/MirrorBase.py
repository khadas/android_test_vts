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

import logging
import os

from vts.runners.host.proto import AndroidSystemControlMessage_pb2
from vts.runners.host.proto import InterfaceSpecificationMessage_pb2
from vts.runners.host.tcp_client import TcpClient

from google.protobuf import text_format


class MirrorBase(object):
  """Base class for all host-side mirror objects.

  This creates a connection to the target-side agent and initializes the
  child mirror class's attributes dynamically.

  Attributes:
    _target_class: string, the target class name (e.g., hal).
    _target_type: string, the target type name (e.g., light, camera).
    _target_version: float, the target component version (e.g., 1.0).
    _target_basepath: string, the path of a base dir which contains the
        target component files.
  """

  _target_class = None
  _target_type = None
  _target_version = None
  _target_basepath = "/system/lib/hw"

  def Init(self):
    """Initializes the connection."""

    logging.info("Init a Mirror for %s", self._target_type)
    self._client = TcpClient.VtsTcpClient()

    self._client.Connect()

    # check whether the binder service is running
    self._client.SendCommand(
        AndroidSystemControlMessage_pb2.CHECK_FUZZER_BINDER_SERVICE,
        "Is fuzzer running?")
    resp = self._client.RecvResponse()
    logging.debug(resp)

    while resp.response_code != AndroidSystemControlMessage_pb2.SUCCESS:
      self._client.SendCommand(
          AndroidSystemControlMessage_pb2.START_FUZZER_BINDER_SERVICE,
          "--server --class=%s --type=%s --version=%s none" % (
          self._target_class, self._target_type, self._target_version))
      resp = self._client.RecvResponse()
      logging.debug(resp)

    self._client.SendCommand(AndroidSystemControlMessage_pb2.GET_HALS,
                             self._target_basepath)
    resp = self._client.RecvResponse()
    logging.debug(resp)

    target_class_id = {"hal": 1}[self._target_class]
    target_type_id = {"light": 4}[self._target_type]

    for filename in resp.reason.strip().split(" "):
      if self._target_type in filename:
        # TODO: check more exactly (e.g., multiple hits).
        self._client.SendCommand(AndroidSystemControlMessage_pb2.SELECT_HAL,
                                 os.path.join(self._target_basepath, filename),
                                 target_class_id, target_type_id, self._target_version)
        resp = self._client.RecvResponse()
        logging.debug(resp)

        if resp.response_code == AndroidSystemControlMessage_pb2.SUCCESS:
          self._client.SendCommand(AndroidSystemControlMessage_pb2.GET_FUNCTIONS,
                                   "get_functions");
          resp = self._client.RecvResponse()
          logging.debug(resp)
          if resp.reason:
            logging.info("len %d", len(resp.reason))
            self._if_spec_msg = InterfaceSpecificationMessage_pb2.InterfaceSpecificationMessage()
            text_format.Merge(resp.reason, self._if_spec_msg)
            logging.debug(self._if_spec_msg)

            self.Build(self._if_spec_msg)
        break

  def Build(self, if_spec_msg=None):
    """Builds the child class's attributes dynamically.

    Args:
      if_spec_msg: InterfaceSpecificationMessage_pb2 proto buf.
    """
    logging.info("Build a Mirror for %s", self._target_type)

    if not if_spec_msg:
      if_spec_msg = self._if_spec_msg

    for api in if_spec_msg.api:
      self.__setattr__(api.name, api)
      logging.debug("setattr %s", api.name)

  def Call(self, api_name, *args, **kwargs):
    """Calls a target component's API.

    Args:
      api_name: string, the name of an API function to call.
      *args: a list of arguments
      **kwargs: a dict for the arg name and value pairs
    """
    func_msg = self.__getattribute__(api_name)

    logging.info(func_msg)
    for arg in func_msg.arg:
      # TODO: use args and kwargs
      if arg.primitive_type == "pointer":
        value = arg.values.add();
        value.pointer = 0
    logging.info(func_msg)

    self._client.SendCommand(AndroidSystemControlMessage_pb2.CALL_FUNCTION,
                             text_format.MessageToString(func_msg));
    resp = self._client.RecvResponse()
    logging.info(resp)
