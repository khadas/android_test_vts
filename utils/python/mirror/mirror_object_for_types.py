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


class MirrorObjectError(Exception):
    """Raised when there is a general error in manipulating a mirror object."""
    pass


class MirrorObjectForTypes(object):
    """The class that can create host-side mirroed variable instances.

    Attributes:
        _if_spec_msg: the interface specification message of a host object to
                      mirror.
        _parent_path: the name of a sub struct this object mirrors.
    """

    def __init__(self, msg, parent_path=None):
        self._if_spec_msg = msg
        self._parent_path = parent_path

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
            VariableSpecificationMessage if found, None otherwise
        """
        try:
            for attribute in self._if_spec_msg.attribute:
                if not attribute.is_const and attribute.name == type_name:
                    return copy.copy(attribute)
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
            VariableSpecificationMessage if found, None otherwise
        """
        try:
            for attribute in self._if_spec_msg.attribute:
                if attribute.is_const and attribute.name == type_name:
                    return copy.copy(attribute)
                elif attribute.type == IfaceSpecMsg.TYPE_ENUM:
                    for enumerator, value in zip(attribute.enum_value.enumerator,
                                                 attribute.enum_value.value):
                        if enumerator == type_name:
                          return copy.copy(attribute)
            return None
        except AttributeError as e:
            # TODO: check in advance whether self._if_spec_msg Interface
            # SpecificationMessage.
            return None

    # TODO: Guard against calls to this function after self.CleanUp is called.
    def __getattr__(self, api_name, *args, **kwargs):
        """Calls a target component's API.

        Args:
            api_name: string, the name of an API function to call.
            *args: a list of arguments
            **kwargs: a dict for the arg name and value pairs
        """
        def MessageGenerator(*args, **kwargs):
            """Dynamically generates a custom message instance."""
            arg_msg = self.GetCustomAggregateType(api_name)
            if not arg_msg:
                raise MirrorObjectError("arg %s unknown" % arg_msg)
            logging.info("MessageGenerator %s %s", api_name, arg_msg)
            logging.debug("MESSAGE %s", api_name)
            if arg_msg.type == IfaceSpecMsg.TYPE_STRUCT:
                for struct_value in arg_msg.struct_value:
                    logging.debug("for %s %s",
                                  struct_value.name, struct_value.scalar_type)
                    first_vector_elem = True
                    for given_name, given_value in kwargs.iteritems():
                        logging.debug("check %s %s", struct_value.name, given_name)
                        if given_name == struct_value.name:
                            logging.debug("match type=%s", struct_value.scalar_type)
                            if struct_value.type == IfaceSpecMsg.TYPE_SCALAR:
                                if struct_value.scalar_type == "uint32_t":
                                    struct_value.scalar_value.uint32_t = given_value
                                elif struct_value.scalar_type == "int32_t":
                                    struct_value.scalar_value.int32_t = given_value
                                else:
                                    raise MirrorObjectError(
                                        "support %s" % struct_value.scalar_type)
                            elif struct_value.type == IfaceSpecMsg.TYPE_VECTOR:
                                sclar_type = struct_value.vector_value[0].scalar_type
                                for value in given_value:
                                    vector_value = struct_value.vector_value[0].value.add()
                                    setattr(vector_value, sclar_type, value)
                            continue
            elif arg_msg.type == IfaceSpecMsg.TYPE_FUNCTION_POINTER:
                for fp_value in arg_msg.function_pointer:
                    logging.debug("for %s", fp_value.function_name)
                    for given_name, given_value in kwargs.iteritems():
                          logging.debug("check %s %s", fp_value.function_name, given_name)
                          if given_name == fp_value.function_name:
                              fp_value.id = self.GetFunctionPointerID(given_value)
                              break

            if arg_msg.type == IfaceSpecMsg.TYPE_STRUCT:
                for struct_value, given_value in zip(arg_msg.struct_value, args):
                    logging.debug("arg match type=%s", struct_value.scalar_type)
                    if struct_value.type == IfaceSpecMsg.TYPE_SCALAR:
                        if struct_value.scalar_type == "uint32_t":
                            struct_value.scalar_value.uint32_t = given_value
                        elif struct_value.scalar_type == "int32_t":
                            struct_value.scalar_value.int32_t = given_value
                        else:
                            raise MirrorObjectError("support %s" % p_type)
            elif arg_msg.type == IfaceSpecMsg.TYPE_FUNCTION_POINTER:
                for fp_value, given_value in zip(arg_msg.function_pointer, args):
                    logging.debug("for %s", fp_value.function_name)
                    fp_value.id = self.GetFunctionPointerID(given_value)
                    logging.debug("fp %s", fp_value)
            logging.debug("generated %s", arg_msg)
            return arg_msg

        def ConstGenerator():
            """Dynamically generates a const variable's value."""
            arg_msg = self.GetConstType(api_name)
            if not arg_msg:
                raise MirrorObjectError("const %s unknown" % arg_msg)
            logging.debug("check %s", api_name)
            if arg_msg.type == IfaceSpecMsg.TYPE_SCALAR:
                ret_v = getattr(arg_msg.scalar_value, arg_msg.scalar_type, None)
                if ret_v is None:
                    raise MirrorObjectError(
                        "No value found for type %s in %s." % (p_type, value))
                return ret_v
            elif arg_msg.type == IfaceSpecMsg.TYPE_STRING:
                return arg_msg.string_value.message
            elif arg_msg.type == IfaceSpecMsg.TYPE_ENUM:
                for enumerator, value in zip(arg_msg.enum_value.enumerator,
                                             arg_msg.enum_value.value):
                    if enumerator == api_name:
                      return value
            raise MirrorObjectError("const %s not found" % api_name)

        struct_msg = self.GetSubStruct(api_name)
        if struct_msg:
            logging.debug("sub_struct %s", struct_msg)
            if self._parent_path:
                parent_name = "%s.%s" % (self._parent_path, api_name)
            else:
                parent_name = api_name
            return MirrorObjectForTypes(struct_msg, parent_path=parent_name)

        # handle attributes.
        arg_msg = self.GetCustomAggregateType(api_name)
        if arg_msg:
            logging.debug("arg %s", arg_msg)
            return MessageGenerator

        arg_msg = self.GetConstType(api_name)
        if arg_msg:
            logging.debug("const %s *\n%s", api_name, arg_msg)
            return ConstGenerator()

        raise MirrorObjectError("unknown api name %s" % api_name)
