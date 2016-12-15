#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
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

import socket
from vts_tcp_server import VtsTcpServer
import unittest
import logging
import errno
from socket import error as socket_error
from vts.runners.host.errors import TcpServerConnectionError
from vts.runners.host.errors import ConnectionRefusedError

HOST, PORT = "localhost", 0
ERROR_PORT = 380 # port at which we test the error case.
counter = 0

class TestMethods(unittest.TestCase):
    """This class defines unit test methods.

    The common scenarios are when we wish to test the whether we are able to
    receive the expected data from the server; and whether we receive the
    correct error when we try to connect to server from a wrong port.

    Attributes:
        _thread_tcp_server: an instance of VtsTcpServer that is used to
                            start and stop the TCP server.
    """
    _thread_tcp_server = None

    def setUp(self):
        """This function initiates starting the server in VtsTcpServer."""

        self._thread_tcp_server = VtsTcpServer()
        self._thread_tcp_server.Start()  # To start the server

    def tearDown(self):
        """To initiate shutdown of the server.

        This function calls the tcp_server.VtsTcpServer.Stop which
        shutdowns the server.
        """
        self._thread_tcp_server.Stop()  # calls tcp_server.VtsTcpServer.Stop

    def DoErrorCase(self):
        """Unit test for Error case.

        This function tests the cases that throw exception.
        e.g sending requests to port 25.

        Raises:
            ConnectionRefusedError: ConnectionRefusedError occurred in
            test_ErrorCase().
        """
        host = self._thread_tcp_server.GetIPAddress()

        # Create a socket (SOCK_STREAM means a TCP socket)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            # Connect to server; this should result in Connection refused error
            sock.connect((host, ERROR_PORT))

        # We are comparing the error number of the error we expect and
        # the error that we get.
        # Test fails if ConnectionRefusedError is not raised at this step.
        except socket_error as e:
            if e.errno == errno.ECONNREFUSED:
                logging.error(e)
                raise ConnectionRefusedError  # Test is a success here
            else:
                raise e  # Test fails, since ConnectionRefusedError was expected

        finally:
            sock.close()

    def ConnectToServer(self, func_id):
        """This function creates a connection to TCP server and sends/receives
            message.

        Args:
            func_id: This is the unique key corresponding to a function and also
                the data that we send to the server.

        Returns:
            received: String corresponding to the message that's received from
                the server.

        Raises:
            TcpServerConnectionError: Exception occurred while stopping server.
        """
        data = func_id
        received = ""

        # Create a socket (SOCK_STREAM means a TCP socket)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        host = self._thread_tcp_server.GetIPAddress()
        port = self._thread_tcp_server.GetPortUsed()
        logging.info('Sending Request to host: %s ,using port: %s ', host, port)

        try:
            # Connect to server and send data
            sock.connect((host, port))
            sock.sendall(data + "\n")

            # Receive data from the server and shut down
            received = sock.recv(1024)
            logging.info('Request sent')
        except socket_error as e:
            logging.error(e)
            raise TcpServerConnectionError('Exception occurred.')
        finally:
            sock.close()
        logging.info('Sent : %s', data)
        logging.info('Received : %s', received)

        return received

    def test_DoErrorCase(self):
        """ Unit test for error cases."""

        self.assertRaises(ConnectionRefusedError, self.DoErrorCase)

    def test_NormalCase(self):
        """Tests the normal request to TCPServer.

        This function sends the request to the Tcp server where the request
        should be a success.

        This function also checks the register callback feature by ensuring that
        callback_func() is called and the value of the global counter is
        increased by one.
        """
        global counter
        func_id = "1"  # to maintain the key for function pointer
        success_response = "Success"

        def callback_func():
            global counter
            counter += 1

        # Function should be registered with RegisterCallback
        self.assertEqual(self._thread_tcp_server.
                         RegisterCallback(func_id, callback_func), True)

        # Capture the previous value of global counter
        prev_value = counter

        # Connect to server
        response = self.ConnectToServer(func_id)

        # Confirm whether the callback_func() was called thereby increasing
        # value of global counter by 1
        self.assertEqual(counter, prev_value + 1)

        # Also confirm if query resulted in a success
        self.assertEqual(response, success_response)

    def test_DoRegisterCallback(self):
        """Checks the register callback functionality of the Server.

        This function checks whether the value of global counter remains same
        if function is not registered. It also checks whether it's incremented
        by 1 when the function is registered.
        """

        global counter
        func_id = "11"  # to maintain the key for function pointer
        error_response = "Error: Function not registered."
        success_response = "Success"

        def callback_func():
            global counter
            counter += 1

        # Capture the previous value of global counter
        prev_value = counter

        # Function should be registered with RegisterCallback
        self.assertEqual(self._thread_tcp_server.
                         RegisterCallback(func_id, callback_func), True)

        # Connect to server
        response = self.ConnectToServer(func_id)

        # Confirm whether the callback_func() was not called.
        self.assertEqual(counter, prev_value + 1)

        # also confirm the error message
        self.assertEqual(response, success_response)

        # Now unregister the function and check again
        # Function should be unregistered with UnegisterCallback
        # and the key should also be present
        self.assertEqual(self._thread_tcp_server.UnregisterCallback(func_id),
                         True)

        # Capture the previous value of global counter
        prev_value = counter

        # Connect to server
        response = self.ConnectToServer(func_id)

        # Confirm whether the callback_func() was not called.
        self.assertEqual(counter, prev_value)

        # also confirm the error message
        self.assertEqual(response, error_response)

if __name__ == '__main__':
    # Test the code
    unittest.main()
