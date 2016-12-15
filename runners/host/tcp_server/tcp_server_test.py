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
from tcp_server import ThreadedTCPServer
import unittest
import logging
import errno
from socket import error as socket_error
from vts.runners.host.errors import TcpServerConnectionError
from vts.runners.host.errors import ConnectionRefusedError

HOST, PORT = "localhost", 0
ERROR_PORT = 380 # port at which we test the error case.

class TestMethods(unittest.TestCase):
    """This class defines unit test methods.

    The common scenarios are when we wish to test the whether we are able to
    receive the expected data from the server; and whether we receive the
    correct error when we try to connect to server from a wrong port.

    Attributes:
        _thread_tcp_server: an instance of ThreadedTCPServer that is used to
                            start and stop the TCP server.
    """
    _thread_tcp_server = None

    def setUp(self):
        """This function initiates starting the server in ThreadedTCPServer."""

        self._thread_tcp_server = ThreadedTCPServer()
        self._thread_tcp_server.Start()  # To start the server

    def tearDown(self):
        """To initiate shutdown of the server.

        This function calls the tcp_server.ThreadedTCPServer.Stop which
        shutdowns the server.
        """
        self._thread_tcp_server.Stop()  # calls tcp_server.ThreadedTCPServer.Stop

    def test_NormalCase(self):
        """Tests the normal request to TCPServer.

        This function sends the request to the Tcp server where the request
        should be a success.

        Raises:
            TcpServerConnectionError: Exception occurred while stopping server.
        """
        data = "This is Google!"
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

    def test_DoErrorCase(self):
        """ Unit test for error cases.
        """
        self.assertRaises(ConnectionRefusedError, self.DoErrorCase)

if __name__ == '__main__':
    # Test the code
    unittest.main()
