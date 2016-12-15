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

import copy
import logging
import random

from vts.utils.python.fuzzer import FuzzerUtils
from vts.proto import InterfaceSpecificationMessage_pb2 as IfaceSpecMsg
from google.protobuf import text_format

# a dict containing the IDs of the registered function pointers.
_function_pointer_id_dict = {}


class MirrorObjectError(Exception):
    """Raised when there is a general error in manipulating a mirror object."""
    pass


class ShellMirrorObject(object):
    """The class that mirrors a shell on the native side.

    Attributes:
        _client: the TCP client instance.
    """

    def __init__(self, client):
        self._client = client

    def Execute(self, command):
        result = self._client.ExecuteShellCommand(command)
        return result

    def CleanUp(self):
        self._client.Disconnect()
