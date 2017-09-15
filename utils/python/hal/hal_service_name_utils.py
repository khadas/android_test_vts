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
from vts.utils.python.hal import hal_info_utils


def GetServiceNamesFromLsHal(dut, hal_service):
    """Get service names for given hal_service using lshal.

    Args:
        dut: the AndroidDevice under test.
        hal_service: string, the FQName of a HAL service, e.g.,
                     android.hardware.foo@1.0::IFoo.

    Returns:
        a list containing all service names for the given HAL.
    """
    cmd = '"lshal -i | grep -o %s/.* | sort -u"' % hal_service
    out = str(dut.adb.shell(cmd)).split()
    service_names = map(lambda x: x[x.find('/') + 1:], out)
    return service_names


def GetServiceNamesFromCompMatrix(comp_matrix_xml, hal_service):
    """Get service names for given hal_service by parsing the
    compatibility_matrix.xml file.

    Args:
        comp_matrix_xml: string, containing a compatiblity_matrix.xml file content.
        hal_service: string, the FQName of a HAL service, e.g.,
                     android.hardware.foo@1.0::IFoo.

    Returns:
        a list containing all service names for the given HAL.
    """
    hals, _ = hal_info_utils.GetHalDescriptions(comp_matrix_xml)
    service_names = GetServiceNames(hals, hal_service)
    return service_names


def GetServiceNamesFromVintf(vintf_xml, hal_service):
    """Get service names for given hal_service by parsing the vintf xml file.

    Args:
        vintf_xml: string, containing a manifest.xml file content.
        hal_service: string, the FQName of a HAL service, e.g.,
                     android.hardware.foo@1.0::IFoo.

    Returns:
        a list containing all service names for the given HAL.
    """
    hwbinder_hals, passthrough_hals = hal_info_utils.GetHalDescriptions(vintf_xml)
    service_names = GetServiceNames(hwbinder_hals, hal_service)
    if len(service_names) == 0:
        # check passthrough hals.
        service_names = GetServiceNames(passthrough_hals, hal_service)
    return service_names


def GetServiceNames(hals, hal_service):
    """Get service names for given hal_service from given HAL dictionaries.

    Args:
        hals: a dictionary containing the information of candidate HALs.
        hal_service: string, the FQName of a HAL service, e.g.,
                     android.hardware.foo@1.0::IFoo.

    Returns:
        a list containing all service names for the given HAL.
    """
    package, interface_name = hal_service.split("::")
    package_name, version = package.split("@")

    service_names = []
    for hal_full_name in hals:
        if hal_info_utils.IsCompatableHal(hals[hal_full_name], package_name,
                                           version):
            for interface in hals[hal_full_name].hal_interfaces:
                if interface.hal_interface_name == interface_name:
                    service_names.extend(interface.hal_interface_instances)
    return service_names
