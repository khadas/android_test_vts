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

from google.protobuf import text_format

from vts.runners.host import errors
from vts.runners.host.proto import InterfaceSpecificationMessage_pb2 as IfaceSpecMsg
from vts.runners.host.tcp_client import vts_tcp_client
from vts.runners.host.tcp_server import vts_tcp_server
from vts.utils.python.mirror import mirror_object

COMPONENT_CLASS_DICT = {"hal_conventional": 1,
                        "sharedlib": 2,
                        "hal_hidl": 3,
                        "hal_submodule": 4,
                        "hal_legacy": 5}

COMPONENT_TYPE_DICT = {"audio": 1,
                       "camera": 2,
                       "gps": 3,
                       "light": 4,
                       "wifi": 5}

VTS_CALLBACK_SERVER_TARGET_SIDE_PORT = 5010

_DEFAULT_TARGET_BASE_PATHS = ["/system/lib64/hw"]


class HalMirror(object):
    """The class that acts as the mirror to an Android device's HAL layer.

    This class holds and manages the life cycle of multiple mirror objects that
    map to different HAL components.

    One can use this class to create and destroy a HAL mirror object.

    Attributes:
        _host_command_port: int, the host-side port for command-response
                            sessions.
        _host_callback_port: int, the host-side port for callback sessions.
        _hal_level_mirrors: dict, key is HAL handler name, value is HAL
                            mirror object.
        _host_port: int, the port number on the host side to use.
        _callback_server: the instance of a callback server.
        _callback_port: int, the port number of a host-side callback server.
    """
    def __init__(self, host_command_port, host_callback_port):
        self._hal_level_mirrors = {}
        self._host_command_port = host_command_port
        self._host_callback_port = host_callback_port
        self._callback_server = None
        self._callback_port = 0

    def __del__(self):
        for hal_mirror_name in self._hal_level_mirrors:
            self.RemoveHal(hal_mirror_name)
        self._hal_level_mirrors = {}

    def InitConventionalHal(self,
                            target_type,
                            target_version,
                            target_basepaths=_DEFAULT_TARGET_BASE_PATHS,
                            handler_name=None,
                            bits=64):
        """Initiates a handler for a particular conventional HAL.

        This will initiate a stub service for a HAL on the target side, create
        the top level mirror object for a HAL, and register it in the manager.

        Args:
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_basepaths: list of strings, the paths to look for target
                             files in. Default is _DEFAULT_TARGET_BASE_PATHS.
            handler_name: string, the name of the handler. target_type is used
                          by default.
            bits: integer, processor architecture indicator: 32 or 64.
        """
        self._CreateMirrorObject("hal_conventional",
                                 target_type,
                                 target_version,
                                 target_basepaths,
                                 handler_name=handler_name,
                                 bits=bits)

    def InitLegacyHal(self,
                      target_type,
                      target_version,
                      target_basepaths=_DEFAULT_TARGET_BASE_PATHS,
                      handler_name=None,
                      bits=64):
        """Initiates a handler for a particular legacy HAL.

        This will initiate a stub service for a HAL on the target side, create
        the top level mirror object for a HAL, and register it in the manager.

        Args:
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_basepaths: list of strings, the paths to look for target
                             files in. Default is _DEFAULT_TARGET_BASE_PATHS.
            handler_name: string, the name of the handler. target_type is used
                          by default.
            bits: integer, processor architecture indicator: 32 or 64.
        """
        self._CreateMirrorObject("hal_legacy",
                                 target_type,
                                 target_version,
                                 target_basepaths,
                                 handler_name=handler_name,
                                 bits=bits)

    def RemoveHal(self, handler_name):
        hal_level_mirror = self._hal_level_mirrors[handler_name]
        hal_level_mirror.CleanUp()

    def _CreateMirrorObject(self,
                            target_class,
                            target_type,
                            target_version,
                            target_basepaths=_DEFAULT_TARGET_BASE_PATHS,
                            handler_name=None,
                            bits=64):
        """Initiates the stub for a HAL on the target device and creates a top
        level MirroObject for it.

        Args:
            target_class: string, the target class name (e.g., hal).
            target_type: string, the target type name (e.g., light, camera).
            target_version: float, the target component version (e.g., 1.0).
            target_basepaths: list of strings, the paths to look for target
                             files in. Default is _DEFAULT_TARGET_BASE_PATHS.
            handler_name: string, the name of the handler. target_type is used
                          by default.
            bits: integer, processor architecture indicator: 32 or 64.

        Raises:
            errors.ComponentLoadingError is raised when error occurs trying to
            create a MirrorObject.
        """
        if bits not in [32, 64]:
            raise error.ComponentLoadingError("Invalid value for bits: %s" % bits)
        client = vts_tcp_client.VtsTcpClient()
        callback_server = vts_tcp_server.VtsTcpServer()
        _, port = callback_server.Start(self._host_callback_port)
        if port != self._host_callback_port:
            raise errors.ComponentLoadingError(
                "Couldn't start a callback TcpServer at port %s" %
                    self._host_callback_port)
        client.Connect(command_port=self._host_command_port,
                       callback_port=self._host_callback_port)
        if not handler_name:
            handler_name = target_type
        service_name = "vts_binder_%s" % handler_name

        # Get all the HALs available on the target.
        hal_list = client.ListHals(target_basepaths)
        if not hal_list:
            raise errors.ComponentLoadingError(
                "Could not find any HAL under path %s" % target_basepaths)
        logging.debug(hal_list)

        # Find the corresponding filename for HAL target type.
        target_filename = None
        for name in hal_list:
            if target_type in name:
                # TODO: check more exactly (e.g., multiple hits).
                target_filename = name
        if not target_filename:
            raise errors.ComponentLoadingError(
                "No file found for HAL target type %s." % target_type)

        # Check whether the requested binder service is already running.
        # if client.CheckStubService(service_name=service_name):
        #     raise errors.ComponentLoadingError("A stub for %s already exists" %
        #                                        service_name)

        # Launch the corresponding stub of the requested HAL on the target.
        logging.info("Init the stub service for %s", target_type)
        target_class_id = COMPONENT_CLASS_DICT[target_class.lower()]
        target_type_id = COMPONENT_TYPE_DICT[target_type.lower()]
        launched = client.LaunchStubService(service_name=service_name,
                                            file_path=target_filename,
                                            bits=bits,
                                            target_class=target_class_id,
                                            target_type=target_type_id,
                                            target_version=target_version)
        if not launched:
            raise errors.ComponentLoadingError(
                "Failed to launch stub service %s from file path %s" %
                (target_type, target_filename))

        # Create API spec message.
        found_api_spec = client.ListApis()
        if not found_api_spec:
            raise errors.ComponentLoadingError("No API found for %s" %
                                               service_name)
        logging.debug("Found %d APIs for %s:\n%s", len(found_api_spec),
                      service_name, found_api_spec)
        if_spec_msg = IfaceSpecMsg.InterfaceSpecificationMessage()
        text_format.Merge(found_api_spec, if_spec_msg)

        # Instantiate a MirrorObject and return it.
        hal_mirror = mirror_object.MirrorObject(client, if_spec_msg,
                                                callback_server)
        self._hal_level_mirrors[handler_name] = hal_mirror

    def __getattr__(self, name):
        return self._hal_level_mirrors[name]
