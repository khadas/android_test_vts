#
# Copyright 2017 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import time
from optparse import OptionParser


FIX_MONTH_RANGE = 4  # Spl convert to make 4 months version


def main():
    # Optparse to image target name, spl month, auto input date
    usage = """usage: Auto change script - Security patch level [option] arg
    e.g) python auto_change_SPL_script.py -t x86_a -m 01 -a """
    parser = OptionParser(usage=usage)
    parser.add_option(
        "-t", "--abtarget", type="string", help='Image build target')
    parser.add_option(
        "-m", "--abmonth", type="int", help='Security patch month')
    parser.add_option(
        "-a", "--alldatecopy", action="store_true", help='Select 01, 05 date')

    (options, args) = parser.parse_args()

    ab_target = options.abtarget
    ab_month = options.abmonth
    all_date_copy = options.alldatecopy
    local_time = time.localtime()
    ab_year = local_time.tm_year

    ConvertGsiSpl(ab_target, ab_month, all_date_copy, ab_year)


def ConvertGsiSpl(ab_target, ab_month, all_date_copy, ab_year):
    # Auto start a change_security_patch_ver.sh
    for month in range(ab_month, ab_month + FIX_MONTH_RANGE):

        output_month = (month - 1) % 12 + 1
        input_year = ab_year + (month - 1) // 12

        if all_date_copy:
            os.system(
                str('./change_security_patch_ver.sh system.img system_aosp_'
                    '%s_%s-%02d-01.img %s-%02d-01' %
                    (ab_target, input_year, output_month, input_year,
                     output_month)))

        os.system(
            str('./change_security_patch_ver.sh system.img system_aosp_'
                '%s_%s-%02d-05.img %s-%02d-05' %
                (ab_target, input_year, output_month, input_year,
                 output_month)))


if __name__ == '__main__':
  main()