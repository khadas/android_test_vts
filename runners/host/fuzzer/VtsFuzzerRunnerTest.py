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

import sys

from logger import Log
from proto import AndroidSystemControlMessage_pb2
from tcp_client import TcpClient


def main(args):
  client = TcpClient.VtsTcpClient()

  client.Connect()

  # check whether the binder service is running
  client.SendCommand(AndroidSystemControlMessage_pb2.CHECK_FUZZER_BINDER_SERVICE,
                     "Is fuzzer running?")
  resp = client.RecvResponse()
  Log.info(resp)

  while resp.response_code != AndroidSystemControlMessage_pb2.SUCCESS:
    client.SendCommand(AndroidSystemControlMessage_pb2.START_FUZZER_BINDER_SERVICE,
                       "--server --class=hal --type=light --version=1.0 "
                       "/system/lib64/hw/lights.angler.so")
    resp = client.RecvResponse()
    Log.info(resp)

  client.SendCommand(AndroidSystemControlMessage_pb2.GET_HALS,
                     "Give me the list of HALs")
  resp = client.RecvResponse()
  Log.info(resp)


if __name__ == "__main__":
  main(sys.argv[1:])
