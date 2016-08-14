#!/usr/bin/env python3.4
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
import time

from vts.runners.host import base_test_with_webdb
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class NfcHidlBasicTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """A simple testcase for the NFC HIDL HAL."""

    def setUpClass(self):
        self.dut = self.registerController(android_device)[0]
        self.dut.hal.InitHidlHal(target_type="nfc",
                                 target_basepaths=["/system/lib64"],
                                 target_version=1.0,
                                 bits=64)

    def testBase(self):
        """A simple testcase which just calls each registered function."""

        # TODO: extend to make realistic testcases
        def send_event(nfc_event_t, nfc_status_t):
            logging.info("callback send_event")
            logging.info("arg0 %s", nfc_event_t)
            logging.info("arg1 %s", nfc_status_t)

        def send_data(nfc_data_t):
            logging.info("callback send_data")
            logging.info("arg0 %s", nfc_data_t)

        client_callback = self.dut.hal.nfc.GetHidlCallbackInterface(
            "INfcClientCallback",
            sendEvent=send_event,
            sendData=send_data)

        result = self.dut.hal.nfc.open(client_callback)
        logging.info("open result: %s",
                     result.return_type.string_value.message)

        result = self.dut.hal.nfc.pre_discover()
        logging.info("pre_discover result: %s",
                     result.return_type.string_value.message)

        result = self.dut.hal.nfc.control_granted()
        logging.info("control_granted result: %s",
                     result.return_type.string_value.message)

        result = self.dut.hal.nfc.power_cycle()
        logging.info("power_cycle result: %s",
                     result.return_type.string_value.message)

        result = self.dut.hal.nfc.core_initialized([0, 9, 8])
        logging.info("core_initialized result: %s",
                     result.return_type.string_value.message)

        nfc_types = self.dut.hal.nfc.GetHidlTypeInterface("types")
        logging.info("nfc_types: %s", nfc_types)
        data_vec = nfc_types.nfc_data_t(data=[0, 1, 2, 3, 4, 5])
        logging.info("data_vec: %s", data_vec)

        result = self.dut.hal.nfc.write(data_vec)
        logging.info("write result: %s",
                     result.return_type.string_value.message)

        result = self.dut.hal.nfc.close()
        logging.info("close result: %s",
                     result.return_type.string_value.message)

        time.sleep(5)


if __name__ == "__main__":
    test_runner.main()
