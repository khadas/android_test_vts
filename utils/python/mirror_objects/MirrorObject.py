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

from vts.runners.host.proto import AndroidSystemControlMessage_pb2
from vts.utils.python.fuzzer import FuzzerUtils
from google.protobuf import text_format


class MirrorObject(object):
    """Actual mirror object.

    Args:
        _client: the TCP client instance.
        _if_spec_msg: the interface specification message of a host object to
            mirror.
    """

    def __init__(self, client, msg):
        self._client = client
        self._if_spec_msg = msg

    def GetApi(self, api_name):
        """Returns the Function Specification Message.

        Args:
            api_name: string, the name of the target function API.

        Returns:
            FunctionSpecificationMessage if found, None otherwise
        """
        for api in self._if_spec_msg.api:
            if api.name == api_name:
                return copy.copy(api)
        return None

    def GetCustomAggregateType(self, type_name):
        """Returns the Argument Specification Message.

        Args:
            type_name: string, the name of the target data type.

        Returns:
            ArgumentSpecificationMessage if found, None otherwise
        """
        for name, definition in zip(self._if_spec_msg.aggregate_type_name,
                                    self._if_spec_msg.aggregate_type_definition):
            if name != "const" and name == type_name:
                return copy.copy(definition)
        return None

    def GetConstType(self, type_name):
        """Returns the Argument Specification Message.

        Args:
            type_name: string, the name of the target const data variable.

        Returns:
            ArgumentSpecificationMessage if found, None otherwise
        """
        for name, definition in zip(self._if_spec_msg.aggregate_type_name,
                                    self._if_spec_msg.aggregate_type_definition):
            if name == "const":
                return copy.copy(definition)
        return None

    def __getattr__(self, api_name, *args, **kwargs):
        """Calls a target component's API.

        Args:
            api_name: string, the name of an API function to call.
            *args: a list of arguments
            **kwargs: a dict for the arg name and value pairs
        """
        def RemoteCall(*args, **kwargs):
            """Dynamically calls a remote API."""
            func_msg = self.GetApi(api_name)
            if not func_msg:
                logging.fatal("api %s unknown", func_msg)

            logging.info("remote call %s", api_name)
            if args:
                for arg_msg, value_msg in zip(func_msg.arg, args):
                    logging.info("arg msg value %s %s", arg_msg, value_msg)
                    if value_msg:
                        for primitive_value in value_msg.primitive_value:
                            pv = arg_msg.primitive_value.add()
                            if primitive_value.HasField("uint32_t"):
                                pv.uint32_t = primitive_value.uint32_t
                            if primitive_value.HasField("int32_t"):
                                pv.int32_t = primitive_value.int32_t
                logging.info("final msg %s", func_msg)
            else:
                # TODO: use kwargs
                for arg in func_msg.arg:
                    # TODO: handle other
                    if arg.primitive_type == "pointer":
                        value = arg.primitive_value.add()
                        value.pointer = 0
                logging.info(func_msg)

            self._client.SendCommand(
                AndroidSystemControlMessage_pb2.CALL_FUNCTION,
                text_format.MessageToString(func_msg))
            resp = self._client.RecvResponse()
            logging.info(resp)

        def MessageGenerator(*args, **kwargs):
            """Dynamically generates a custom message instance."""
            arg_msg = self.GetCustomAggregateType(api_name)
            if not arg_msg:
                logging.fatal("arg %s unknown", arg_msg)

            for type, name, value in zip(arg_msg.primitive_type,
                                         arg_msg.primitive_name,
                                         arg_msg.primitive_value):
                logging.debug("for %s %s %s", type, name, value)
                # todo handle args too
                for given_name, given_value in kwargs.iteritems():
                    logging.debug("check %s %s", name, given_name)
                    if given_name == name:
                        logging.debug("match")
                        if type == "uint32_t":
                            value.uint32_t = given_value
                        elif type == "int32_t":
                            value.int32_t = given_value
                        else:
                            logging.fatal("support %s", type)
                        continue
            logging.debug("generated %s", arg_msg)
            return arg_msg

        def MessageFuzzer(arg_msg):
            """Fuzz a custom message instance."""
            if not self.GetCustomAggregateType(api_name):
                logging.fatal("fuzz arg %s unknown", arg_msg)

            index = random.randint(0, len(arg_msg.primitive_type))
            count = 0
            for type, name, value in zip(arg_msg.primitive_type,
                                         arg_msg.primitive_name,
                                         arg_msg.primitive_value):
                if count == index:
                    if type == "uint32_t":
                        value.uint32_t ^= FuzzerUtils.mask_uint32_t()
                    elif type == "int32_t":
                        mask = FuzzerUtils.mask_int32_t()
                        if mask == (1 << 31):
                            value.int32_t *= -1
                            value.int32_t += 1
                        else:
                            value.int32_t ^= mask
                    else:
                        logging.fatal("support %s", type)
                    break
                count += 1
            logging.debug("fuzzed %s", arg_msg)
            return arg_msg

        def ConstGenerator():
            """Dynamically generates a const variable's value."""
            arg_msg = self.GetConstType(api_name)
            if not arg_msg:
                logging.fatal("const %s unknown", arg_msg)
            logging.debug("check %s", api_name)
            for type, name, value in zip(arg_msg.primitive_type,
                                         arg_msg.primitive_name,
                                         arg_msg.primitive_value):
                logging.debug("for %s %s %s", type, name, value)
                if api_name == name:
                    logging.debug("match")
                    if type == "uint32_t":
                        logging.info("return %s", value)
                        return value.uint32_t
                    elif type == "int32_t":
                        logging.info("return %s", value)
                        return value.int32_t
                    elif type == "bytes":
                        logging.info("return %s", value)
                        return value.bytes
                    else:
                        logging.fatal("support %s", type)
                    continue
            logging.fatal("const %s not found", arg_msg)

        func_msg = self.GetApi(api_name)
        if func_msg:
            logging.info("api %s", func_msg)
            return RemoteCall

        fuzz = False
        if api_name.endswith("_fuzz"):
          fuzz = True
          api_name = api_name[:-5]
        arg_msg = self.GetCustomAggregateType(api_name)
        if arg_msg:
            logging.debug("arg %s", arg_msg)
            if not fuzz:
                return MessageGenerator
            else:
                return MessageFuzzer

        arg_msg = self.GetConstType(api_name)
        if arg_msg:
            logging.info("const %s *\n%s", api_name, arg_msg)
            return ConstGenerator()
        logging.fatal("unknown api name %s", api_name)

