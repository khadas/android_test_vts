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

import json

from vts.runners.host import asserts
from vts.runners.host import const

VTS_TESTABILITY_CHECKER_32 = "/data/local/tmp/vts_testability_checker32"
VTS_TESTABILITY_CHECKER_64 = "/data/local/tmp/vts_testability_checker64"

def GetHalServiceName(shell, hal, bitness="64", run_as_compliance_test=False):
    """Determine whether to run a VTS test against a HAL and get the service
    names of the given hal if determine to run.

    Args:
        shell: the ShellMirrorObject to execute command on the device.
        hal: string, the FQName of a HAL service, e.g.,
                     android.hardware.foo@1.0::IFoo
        bitness: string, the bitness of the test.
        run_as_compliance_test: boolean, whether it is a compliance test.

    Returns:
        a boolean whether to run the test against the given hal.
        a set containing all service names for the given HAL.
    """

    cmd = VTS_TESTABILITY_CHECKER_64
    if bitness == "32":
        cmd = VTS_TESTABILITY_CHECKER_32
    if run_as_compliance_test:
        cmd += " -c "
    cmd += " -b " + bitness + " " + hal
    cmd_results = shell.Execute(str(cmd))
    asserts.assertFalse(
            any(cmd_results[const.EXIT_CODE]),
            "Failed to run vts_testability_checker.")
    result = json.loads(cmd_results[const.STDOUT][0])
    if str(result['Testable']).lower() == "true":
        return True, set(result['instances'])
    else:
        return False, ()


def GetServiceInstancesCombinations(services, service_instances):
    """Create all combinations of instances for all services.

    Args:
        services: list, all services used in the test. e.g. [s1, s2]
        service_instances: dictionary, mapping of each service and the
                           corresponding service name(s).
                           e.g. {"s1": ["n1"], "s2": ["n2", "n3"]}

    Returns:
        A list of all service instance combinations.
        e.g. [[s1/n1, s2/n2], [s1/n1, s2/n3]]
    """
    service_instance_combinations = []
    if not services or (service_instances and type(service_instances) != dict):
        return service_instance_combinations
    service = services.pop()
    pre_instance_combs = GetServiceInstancesCombinations(services,
                                                         service_instances)
    if service not in service_instances or not service_instances[service]:
        return pre_instance_combs
    for name in service_instances[service]:
        if not pre_instance_combs:
            new_instance_comb = [service + '/' + name]
            service_instance_combinations.append(new_instance_comb)
        else:
            for instance_comb in pre_instance_combs:
                new_instance_comb = [service + '/' + name]
                new_instance_comb.extend(instance_comb)
                service_instance_combinations.append(new_instance_comb)

    return service_instance_combinations
