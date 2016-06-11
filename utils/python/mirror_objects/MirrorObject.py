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
from vts.runners.host.proto import InterfaceSpecificationMessage_pb2
from google.protobuf import text_format


# a dict containing the IDs of the registered function pointers.
_function_pointer_id_dict = {}


class MirrorObject(object):
    """Actual mirror object.

    Args:
        _client: the TCP client instance.
        _if_spec_msg: the interface specification message of a host object to
            mirror.
        _parent_path: the name of a sub struct this object mirrors.
    """

    def __init__(self, client, msg, parent_path=None):
        self._client = client
        self._if_spec_msg = msg
        self._parent_path = parent_path

    def GetFunctionPointerID(self, function_pointer):
        """Returns the function pointer ID for the given one."""
        max = 0
        for key in _function_pointer_id_dict:
            if _function_pointer_id_dict[key] == function_pointer:
                return _function_pointer_id_dict[function_pointer]
            if not max or key > max:
                max = key
        _function_pointer_id_dict[max + 1] = function_pointer
        return str(max + 1)

    def Open(self, module_name=None):
        """Opens the target HAL component (only for conventional HAL).

        Args:
            module_name: string, the name of a module to load.
        """
        func_msg = InterfaceSpecificationMessage_pb2.FunctionSpecificationMessage()
        func_msg.name = "#Open"
        logging.info("remote call %s", func_msg.name)
        if module_name:
            arg = func_msg.arg.add()
            arg.primitive_type.append("string")
            value = arg.primitive_value.add()
            value.bytes = module_name
            func_msg.return_type.primitive_type.append("int32_t")
        logging.info("final msg %s", func_msg)

        result = self._client.CallApi(text_format.MessageToString(func_msg))
        logging.info(result)
        return result

    def GetApi(self, api_name):
        """Returns the Function Specification Message.

        Args:
            api_name: string, the name of the target function API.

        Returns:
            FunctionSpecificationMessage if found, None otherwise
        """
        logging.debug("GetAPI %s for %s", api_name, self._if_spec_msg)
        if self._if_spec_msg.api:
            for api in self._if_spec_msg.api:
                if api.name == api_name:
                    return copy.copy(api)
        return None

    def GetSubStruct(self, sub_struct_name):
        """Returns the Struct Specification Message.

        Args:
            sub_struct_name: string, the name of the target sub struct attribute.

        Returns:
            StructSpecificationMessage if found, None otherwise
        """
        if self._if_spec_msg.sub_struct:
            for sub_struct in self._if_spec_msg.sub_struct:
                if sub_struct.name == sub_struct_name:
                    return copy.copy(sub_struct)
        return None

    def GetCustomAggregateType(self, type_name):
        """Returns the Argument Specification Message.

        Args:
            type_name: string, the name of the target data type.

        Returns:
            ArgumentSpecificationMessage if found, None otherwise
        """
        try:
            for name, definition in zip(self._if_spec_msg.aggregate_type_name,
                                        self._if_spec_msg.aggregate_type_definition):
                if name != "const" and name == type_name:
                    return copy.copy(definition)
            return None
        except AttributeError as e:
            # TODO: check in advance whether self._if_spec_msg Interface
            # SpecificationMessage.
            return None

    def GetConstType(self, type_name):
        """Returns the Argument Specification Message.

        Args:
            type_name: string, the name of the target const data variable.

        Returns:
            ArgumentSpecificationMessage if found, None otherwise
        """
        try:
            for name, definition in zip(self._if_spec_msg.aggregate_type_name,
                                        self._if_spec_msg.aggregate_type_definition):
                if name == "const":
                    return copy.copy(definition)
            return None
        except AttributeError as e:
            # TODO: check in advance whether self._if_spec_msg Interface
            # SpecificationMessage.
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

            logging.info("remote call %s.%s", self._parent_path, api_name)
            logging.debug("remote call %s%s", api_name, args)
            if args:
                for arg_msg, value_msg in zip(func_msg.arg, args):
                    logging.debug("arg msg value %s %s", arg_msg, value_msg)
                    if value_msg is not None:
                        # check whether value_msg is a message
                        # value_msg.HasField("primitive_value")
                        if isinstance(value_msg, int):
                            pv = arg_msg.primitive_value.add()
                            pv.int32_t = value_msg
                        else:
                            # TODO: check in advance (whether it's a message)
                            if isinstance(value_msg,
                                          InterfaceSpecificationMessage_pb2.ArgumentSpecificationMessage):
                              arg_msg.CopyFrom(value_msg)
                              arg_msg.ClearField("primitive_value")
                            else:
                              logging.error("unknown type %s", type(value_msg))

                            try:
                                for primitive_value in value_msg.primitive_value:
                                    pv = arg_msg.primitive_value.add()
                                    if primitive_value.HasField("uint32_t"):
                                        pv.uint32_t = primitive_value.uint32_t
                                    if primitive_value.HasField("int32_t"):
                                        pv.int32_t = primitive_value.int32_t
                                    if primitive_value.HasField("bytes"):
                                        pv.bytes = primitive_value.bytes
                            except AttributeError as e:
                                logging.exception(e)
                                raise
                logging.info("final msg %s", func_msg)
            else:
                # TODO: use kwargs
                for arg in func_msg.arg:
                    # TODO: handle other
                    if arg.primitive_type == "pointer":
                        value = arg.primitive_value.add()
                        value.pointer = 0
                logging.debug(func_msg)

            if self._parent_path:
              func_msg.parent_path = self._parent_path
            result = self._client.CallApi(text_format.MessageToString(func_msg))
            logging.debug(result)
            return result

        def MessageGenerator(*args, **kwargs):
            """Dynamically generates a custom message instance."""
            arg_msg = self.GetCustomAggregateType(api_name)
            if not arg_msg:
                logging.fatal("arg %s unknown", arg_msg)
            logging.info("MESSAGE %s", api_name)
            for type, name, value in zip(arg_msg.primitive_type,
                                         arg_msg.primitive_name,
                                         arg_msg.primitive_value):
                logging.debug("for %s %s %s", type, name, value)
                for given_name, given_value in kwargs.iteritems():
                    logging.info("check %s %s", name, given_name)
                    if given_name == name:
                        logging.info("match type=%s", type)
                        if type == "uint32_t":
                            value.uint32_t = given_value
                        elif type == "int32_t":
                            value.int32_t = given_value
                        elif type == "function_pointer":
                            value.bytes = self.GetFunctionPointerID(given_value)
                        else:
                            logging.fatal("support %s", type)
                        continue
            for type, name, value, given_value in zip(arg_msg.primitive_type,
                                                      arg_msg.primitive_name,
                                                      arg_msg.primitive_value,
                                                      args):
                logging.info("arg match type=%s", type)
                if type == "uint32_t":
                    value.uint32_t = given_value
                elif type == "int32_t":
                    value.int32_t = given_value
                elif type == "function_pointer":
                    value.bytes = self.GetFunctionPointerID(given_value)
                else:
                    logging.fatal("support %s", type)
                continue
            logging.info("generated %s", arg_msg)
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
            logging.info("fuzzed %s", arg_msg)
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

        # handle APIs.
        func_msg = self.GetApi(api_name)
        if func_msg:
            logging.debug("api %s", func_msg)
            return RemoteCall

        struct_msg = self.GetSubStruct(api_name)
        if struct_msg:
            logging.debug("sub_struct %s", struct_msg)
            if self._parent_path:
                parent_name = "%s.%s" % (self._parent_path, api_name)
            else:
                parent_name = api_name
            return MirrorObjectForSubStruct(self._client, struct_msg,
                                            parent_name)

        # handle attributes.
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


class MirrorObjectForSubStruct(MirrorObject):
    """Actual mirror object for sub struct.

    Args:
        _client: see MirrorObject
        _if_spec_msg: see MirrorObject
        _name: see MirrorObject
    """
