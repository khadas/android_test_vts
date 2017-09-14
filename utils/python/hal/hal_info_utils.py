#
# Copyright (C) 2017 The Android Open Source Project
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

from xml.etree import ElementTree

_ARCH = "arch"
_HWBINDER = "hwbinder"
_NAME = "name"
_PASSTHROUGH = "passthrough"
_TRANSPORT = "transport"
_VERSION = "version"
_INTERFACE = "interface"
_INSTANCE = "instance"


def ParseHalVersion(hal_version):
    """Returns major and minor verions extracted from hal_version string.

    Args:
        hal_version: string, HAL version (e.g., "1.2")

    Returns:
        integer: major version
        integer: minor version

    Raises:
        ValueError if malformed version string is given.
    """
    hal_version_major, hal_version_minor = hal_version.split(".")
    return int(hal_version_major), int(hal_version_minor)


def IsCompatableHal(hal_desc, package_name, version):
    """Check whether the HAL info is compatable with given package and version.

    Args:
        hal_desc: a HalDescription object containing a HAL info.
        package_name: string, HAL package name e.g., android.hardware.foo
        version: string HAL version e.g., 1.0.

    Returns:
        True if the hal_desc is compatable with the given package and version.
    """
    major_version, minor_version = ParseHalVersion(version)
    if (hal_desc.hal_name == package_name and
            hal_desc.hal_version_major == major_version and
            hal_desc.hal_version_minor >= minor_version):
        return True
    return False


class HalInterfaceDescription(object):
    """Class to store the information of a running hal service interface.

    Attributes:
        hal_interface_name: interface name within the hal e.g. INfc.
        hal_instance_instances: a list of instance name of the registered hal
                                service e.g. default. nfc
    """

    def __init__(self, hal_interface_name, hal_interface_instances):
        self.hal_interface_name = hal_interface_name
        self.hal_interface_instances = hal_interface_instances


class HalDescription(object):
    """Class to store the information of a running hal service.

    Attributes:
        hal_name: hal name e.g. android.hardware.nfc.
        hal_version: string, hal version e.g. 1.0.
        hal_version_major: integer, major version.
        hal_version_minor: integer, minor version.
        hal_interfaces: a list of HalInterfaceDescription within the hal.
        hal_archs: a list of strings where each string indicates the supported
                   client bitness (e.g,. ["32", "64"]).
        hal_key: string, key to identify the HAL.
    """

    def __init__(self, hal_name, hal_version, hal_interfaces, hal_archs):
        self.hal_name = hal_name
        self.hal_version = hal_version
        if '-' in hal_version:
            low_version, high_version_minor = hal_version.split('-')
            low_version_major, _ = ParseHalVersion(low_version)
            self.hal_version_major = low_version_major
            self.hal_version_minor = high_version_minor
        elif "." in hal_version:
            self.hal_version_major, self.hal_version_minor = ParseHalVersion(
                hal_version)
        else:
            self.hal_version_major = -1
            self.hal_version_minor = -1
        self.hal_key = "%s@%s.%s" % (self.hal_name, self.hal_version_major,
                                     self.hal_version_minor)
        self.hal_interfaces = hal_interfaces
        self.hal_archs = hal_archs


def GetHalDescriptions(xml_file):
    """Parses a vintf xml string.

    Args:
        vintf_xml: string, containing a vintf.xml file content.

    Returns:
        a dictionary containing the information of hwbinder HALs,
        a dictionary containing the information of passthrough HALs.
    """
    try:
        xml_root = ElementTree.fromstring(xml_file)
    except ElementTree.ParseError as e:
        logging.exception(e)
        logging.error('This vintf xml could not be parsed:\n%s' % xml_file)
        return None, None

    hwbinder_hals = dict()
    passthrough_hals = dict()

    for xml_hal in xml_root:
        if xml_hal.tag != 'hal':
            logging.debug('xml file has a non-hal child with tag: %s',
                          xml_hal.tag)
            continue
        hal_name = None
        hal_transport = None
        hal_versions = []
        hal_interfaces = []
        hal_archs = ["32", "64"]
        for xml_hal_item in xml_hal:
            tag = str(xml_hal_item.tag)
            if tag == _NAME:
                hal_name = str(xml_hal_item.text)
            elif tag == _TRANSPORT:
                hal_transport = str(xml_hal_item.text)
                if _ARCH in xml_hal_item.attrib:
                    hal_archs = xml_hal_item.attrib[_ARCH].split("+")
            elif tag == _VERSION:
                # It is possible to have multiple version tags.
                hal_versions.append(str(xml_hal_item.text))
            elif tag == _INTERFACE:
                hal_interface_name = None
                hal_interface_instances = []
                for interface_item in xml_hal_item:
                    tag = str(interface_item.tag)
                    if tag == _NAME:
                        hal_interface_name = str(interface_item.text)
                    elif tag == _INSTANCE:
                        hal_interface_instances.append(
                            str(interface_item.text))
                hal_interfaces.append(
                    HalInterfaceDescription(hal_interface_name,
                                            hal_interface_instances))

        # Create hal description for each version.
        for version in hal_versions:
            hal_info = HalDescription(hal_name, version, hal_interfaces,
                                      hal_archs)
            if hal_transport is None or hal_transport == _HWBINDER:
                hwbinder_hals[hal_info.hal_key] = hal_info
            elif hal_transport == _PASSTHROUGH:
                passthrough_hals[hal_info.hal_key] = hal_info
            else:
                logging.error("Unknown transport type %s", hal_transport)

    return hwbinder_hals, passthrough_hals
