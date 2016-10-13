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


from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class NfcHidlBasicTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """A simple testcase for the NFC HIDL HAL."""

    _TREBLE_DEVICE_NAME_SUFFIX = "_treble"

    def setUpClass(self):
        """Creates a mirror and turns on the framework-layer NFC service."""
        self.dut = self.registerController(android_device)[0]
        self.dut.hal.InitHidlHal(target_type="nfc",
                                 target_basepaths=["/system/lib64"],
                                 target_version=1.0,
                                 bits=64)

        self.dut.shell.InvokeTerminal("one")
        self.dut.shell.one.Execute("setenforce 0")  # SELinux permissive mode
        self.dut.shell.one.Execute("service call nfc 4")  # Turn off
        self.dut.shell.one.Execute("service call nfc 5")  # Turn on

    def tearDownClass(self):
        """Turns off the framework-layer NFC service."""
        self.dut.shell.one.Execute("service call nfc 4")

    def testBase(self):
        """A simple test case which just calls each registered function."""
        asserts.skipIf(
            not self.dut.model.endswith(self._TREBLE_DEVICE_NAME_SUFFIX),
            "a non-Treble device.")

        # TODO: extend to make realistic testcases
        def send_event(NfcEvent, NfcStatus):
            logging.info("callback send_event")
            logging.info("arg0 %s", NfcEvent)
            logging.info("arg1 %s", NfcStatus)

        def send_data(NfcData):
            logging.info("callback send_data")
            logging.info("arg0 %s", NfcData)

        client_callback = self.dut.hal.nfc.GetHidlCallbackInterface(
            "INfcClientCallback",
            sendEvent=send_event,
            sendData=send_data)

        result = self.dut.hal.nfc.open(client_callback)
        logging.info("open result: %s", result)

        result = self.dut.hal.nfc.prediscover()
        logging.info("prediscover result: %s", result)

        result = self.dut.hal.nfc.controlGranted()
        logging.info("controlGranted result: %s", result)

        result = self.dut.hal.nfc.powerCycle()
        logging.info("powerCycle result: %s", result)

        result = self.dut.hal.nfc.coreInitialized([0, 9, 8])
        logging.info("coreInitialized result: %s", result)

        nfc_types = self.dut.hal.nfc.GetHidlTypeInterface("types")
        logging.info("nfc_types: %s", nfc_types)

        result = self.dut.hal.nfc.write(data_vec)
        logging.info("write result: %s", result)

        result = self.dut.hal.nfc.close()
        logging.info("close result: %s", result)

        time.sleep(5)


if __name__ == "__main__":
    test_runner.main()
