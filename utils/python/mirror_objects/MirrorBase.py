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

from vts.runners.host import errors
from vts.runners.host.proto import AndroidSystemControlMessage_pb2
from vts.runners.host.proto import InterfaceSpecificationMessage_pb2
from vts.runners.host.tcp_client import TcpClient
from vts.utils.python.mirror_objects import MirrorObject

from google.protobuf import text_format

COMPONENT_CLASS_DICT = {"hal": 1,
                        "sharedlib": 2,
                        "hal_hidl": 3,
                        "hal_submodule": 4,
                        "legacy_hal": 5}

COMPONENT_TYPE_DICT = {"audio": 1,
                       "camera": 2,
                       "gps": 3,
                       "light": 4,
                       "wifi": 5}


class MirrorBase(object):
    """Base class for all host-side mirror objects.

    This creates a connection to the target-side agent and initializes the
    child mirror class's attributes dynamically.

    Attributes:
        _target_basepath: string, the path of a base dir which contains the
            target component files.
    """

    _target_basepath = ["/system/lib64/hw"]

    def Init(self, target_class, target_type, target_version, target_basepath,
             module_name=None, handler_name=None, bits=64):
        """Initializes the connection and then calls 'Build' to init attributes.

        Args:
            target_class: string, the target class name (e.g., hal).
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_basepath: string, the base path of where a target file is
                stored in.
            module_name: string, the name of a module to load.
            handler_name: string, the name of the handler.
                by default, target_type is used.

        Raises:
            ComponentLoadingError when loading fails.
        """
        if not target_basepath:
            target_basepath = self._target_basepath
        if not handler_name:
            handler_name = target_type

        logging.info("Init a Mirror for %s", target_type)
        self._client = TcpClient.VtsTcpClient()

        self._client.Connect()

        logging.info("target basepath: %s", target_basepath)
        listed_hals = self._client.ListHals(target_basepath)
        logging.debug(listed_hals)

        found_target_filename = None
        if listed_hals:
          for hal_filename in listed_hals:
              if target_type in hal_filename:
                  # TODO: check more exactly (e.g., multiple hits).
                  found_target_filename = hal_filename
                  break

        if not found_target_filename:
          logging.error("no target component found %s", target_type)
          return

        # check whether the binder service is running
        if not self._client.CheckStubService(service_name=handler_name):
            # consider doing: raise errors.ComponentLoadingError(
            #    "A stub for %s already exists" % handler_name)
            target_class_id = COMPONENT_CLASS_DICT[target_class.lower()]
            target_type_id = COMPONENT_TYPE_DICT[target_type.lower()]

            launched = self._client.LaunchStubService(
                service_name=handler_name, file_path=found_target_filename,
                bits=bits, target_class=target_class_id,
                target_type=target_type_id, target_version=target_version,
                module_name=module_name)
            if not launched:
                raise errors.ComponentLoadingError(
                    "Target file path %s" % filename)

        found_api_spec = self._client.ListApis()
        logging.debug("ListApis: %s", found_api_spec)
        if found_api_spec:
            logging.debug("len %d", len(found_api_spec))
            self._if_spec_msg = InterfaceSpecificationMessage_pb2.InterfaceSpecificationMessage()
            text_format.Merge(found_api_spec, self._if_spec_msg)
            self.Build(target_type, self._if_spec_msg)

    def Build(self, target_type, if_spec_msg=None):
        """Builds the child class's attributes dynamically.

        Args:
            target_name: string, the name of the target mirror to create.
            if_spec_msg: InterfaceSpecificationMessage_pb2 proto buf.
        """
        logging.info("Build a Mirror for %s", target_type)
        if not if_spec_msg:
            if_spec_msg = self._if_spec_msg

        logging.info(if_spec_msg)
        mirror_object = MirrorObject.MirrorObject(self._client, if_spec_msg)
        self.__setattr__(target_type, mirror_object)
