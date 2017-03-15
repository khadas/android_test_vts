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

_HWBINDER = "hwbinder"
_NAME = "name"
_PASSTHROUGH = "passthrough"
_TRANSPORT = "transport"
_VERSION = "version"

def GetHalNamesAndVersions(vintf_xml):
    """Parses a vintf xml string.

    Args:
        vintf_xml: string, containing a vintf.xml file content.

    Returns:
        a list of tuples containing the (name, version) of hwbinder HALs,
        a list of tuples containing the (name, version) of passthrough HALs.
    """
    try:
        xml_root = ElementTree.fromstring(vintf_xml)
    except ElementTree.ParseError as e:
        logging.exception(e)
        logging.error('This vintf xml could not be parsed:\n%s' % vintf_xml)
        return None, None

    hwbinder_hals = []
    passthrough_hals = []

    for xml_hal in xml_root:
        hal_name = None
        hal_transport = None
        hal_version = None
        for xml_hal_item in xml_hal:
            tag = str(xml_hal_item.tag)
            if tag == _NAME:
                hal_name = str(xml_hal_item.text)
            elif tag == _TRANSPORT:
                hal_transport = str(xml_hal_item.text)
            elif tag == _VERSION:
                hal_version = str(xml_hal_item.text)
        if hal_transport == _HWBINDER:
            hwbinder_hals.append((hal_name, hal_version))
        elif hal_transport == _PASSTHROUGH:
            passthrough_hals.append((hal_name, hal_version))
        else:
            logging.error("Unknown transport type %s", hal_transport)
    return hwbinder_hals, passthrough_hals
