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
import sys

from logger import Log
from proto import AndroidSystemControlMessage_pb2
from proto import InterfaceSpecificationMessage_pb2
from google.protobuf import text_format
from tcp_client import TcpClient
from sys import api_version


def main(args):
  Log.SetupLogger()

  client = TcpClient.VtsTcpClient()

  client.Connect()

  # check whether the binder service is running
  client.SendCommand(AndroidSystemControlMessage_pb2.CHECK_FUZZER_BINDER_SERVICE,
                     "Is fuzzer running?")
  resp = client.RecvResponse()
  logging.info(resp)

  while resp.response_code != AndroidSystemControlMessage_pb2.SUCCESS:
    client.SendCommand(AndroidSystemControlMessage_pb2.START_FUZZER_BINDER_SERVICE,
                       "--server --class=hal --type=light --version=1.0 "
                       "none")
    resp = client.RecvResponse()
    logging.info(resp)

  client.SendCommand(AndroidSystemControlMessage_pb2.GET_HALS,
                     "/system/lib/hw/")  # /system/lib64/hw/lights.angler.so
  resp = client.RecvResponse()
  logging.info(resp)

  for filename in resp.reason.strip().split(" "):
    if "light" in filename:
      client.SendCommand(AndroidSystemControlMessage_pb2.SELECT_HAL,
                         os.path.join("/system/lib/hw/", filename),
                         1,  # HAL
                         4,  # LIGHT
                         1.0)
      resp = client.RecvResponse()
      logging.info(resp)

      if resp.response_code == AndroidSystemControlMessage_pb2.SUCCESS:
        client.SendCommand(AndroidSystemControlMessage_pb2.GET_FUNCTIONS,
                           "get_functions");
        resp = client.RecvResponse()
        logging.info(resp)
        if resp.reason:
          logging.info("len %d", len(resp.reason))
          msg = InterfaceSpecificationMessage_pb2.InterfaceSpecificationMessage()
          text_format.Merge(resp.reason, msg)
          logging.info(msg)

          # (form a request with values)
          selected_api = None
          for api in msg.api:
            logging.info(api.name)
            for arg in api.arg:
              if arg.primitive_type == "pointer":
                value = arg.values.add();
                value.pointer = 0
            selected_api = api
            break
          logging.info(msg)

          # call the first api
          if selected_api:
            client.SendCommand(AndroidSystemControlMessage_pb2.CALL_FUNCTION,
                               text_format.MessageToString(selected_api));
            resp = client.RecvResponse()
            logging.info(resp)
      break


if __name__ == "__main__":
  main(sys.argv[1:])
