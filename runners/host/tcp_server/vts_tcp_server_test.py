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
import vts_tcp_server
import unittest
import logging
import errno
from socket import error as socket_error
from vts.runners.host.errors import TcpServerConnectionError
from vts.runners.host.errors import ConnectionRefusedError
from vts.runners.host.proto import AndroidSystemControlMessage_pb2 as SysMsg_pb2

HOST, PORT = "localhost", 0
ERROR_PORT = 380 # port at which we test the error case.

class TestMethods(unittest.TestCase):
    """This class defines unit test methods.

    The common scenarios are when we wish to test the whether we are able to
    receive the expected data from the server; and whether we receive the
    correct error when we try to connect to server from a wrong port.

    Attributes:
        _vts_tcp_server: an instance of VtsTcpServer that is used to
                            start and stop the TCP server.
        counter: This is used to keep track of number of calls made to the
            callback function.
    """
    _vts_tcp_server = None
    counter = 0

    def setUp(self):
        """This function initiates starting the server in VtsTcpServer."""

        self._vts_tcp_server = vts_tcp_server.VtsTcpServer()
        self._vts_tcp_server.Start()  # To start the server

    def tearDown(self):
        """To initiate shutdown of the server.

        This function calls the vts_tcp_server.VtsTcpServer.Stop which
        shutdowns the server.
        """
        self._vts_tcp_server.Stop()  # calls vts_tcp_server.VtsTcpServer.Stop

    def DoErrorCase(self):
        """Unit test for Error case.

        This function tests the cases that throw exception.
        e.g sending requests to port 25.

        Raises:
            ConnectionRefusedError: ConnectionRefusedError occurred in
            test_ErrorCase().
        """
        host = self._vts_tcp_server.GetIPAddress()

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
            func_id: This is the unique key corresponding to a function and
                also the id field of the request_message that we send to the
                server.

        Returns:
            response_message: The object that the TCP host returns.

        Raises:
            TcpServerConnectionError: Exception occurred while stopping server.
        """

        # This object is sent to the TCP host
        request_message = SysMsg_pb2.AndroidSystemCallbackRequestMessage()
        request_message.id = func_id

        #  The response in string format that we receive from host
        received_message = ""

        # The final object that this function returns
        response_message = SysMsg_pb2.AndroidSystemCallbackResponseMessage()

        # Create a socket (SOCK_STREAM means a TCP socket)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        host = self._vts_tcp_server.GetIPAddress()
        port = self._vts_tcp_server.GetPortUsed()
        logging.info('Sending Request to host: %s ,using port: %s ', host, port)

        try:
            # Connect to server and send request_message
            sock.connect((host, port))
            message = request_message.SerializeToString()

            logging.info("sending a AndroidSystemCallbackRequestMessage()")
            sock.sendall(message)  # Send the message
            logging.info('Sent : %s', request_message)

            # Receive request_message from the server and shut down
            received_message = sock.recv(1024)
            response_message.ParseFromString(received_message)
            logging.info('Received : %s', received_message)

        except socket_error as e:
            logging.error(e)
            raise TcpServerConnectionError('Exception occurred.')

        finally:
            sock.close()

        return response_message

    def testDoErrorCase(self):
        """ Unit test for error cases."""
        with self.assertRaises(ConnectionRefusedError):
            self.DoErrorCase()


    def testNormalCase(self):
        """Tests the normal request to TCPServer.

        This function sends the request to the Tcp server where the request
        should be a success.

        This function also checks the register callback feature by ensuring that
        callback_func() is called and the value of the global counter is
        increased by one.
        """
        self.counter
        func_id = "1"  # to maintain the key for function pointer

        def callback_func():
            self.counter
            self.counter += 1

        # Function should be registered with RegisterCallback
        self.assertEqual(self._vts_tcp_server.
                         RegisterCallback(func_id, callback_func), True)

        # Capture the previous value of global counter
        prev_value = self.counter

        # Connect to server
        response_message = self.ConnectToServer(func_id)

        # Confirm whether the callback_func() was called thereby increasing
        # value of global counter by 1
        self.assertEqual(self.counter, prev_value + 1)

        # Also confirm if query resulted in a success
        self.assertEqual(response_message.response_code, SysMsg_pb2.SUCCESS)

    def testDoRegisterCallback(self):
        """Checks the register callback functionality of the Server.

        This function checks whether the value of global counter remains same
        if function is not registered. It also checks whether it's incremented
        by 1 when the function is registered.
        """

        self.counter
        func_id = "11"  # to maintain the key for function pointer

        def callback_func():
            self.counter
            self.counter += 1

        # Capture the previous value of global counter
        prev_value = self.counter

        # Function should be registered with RegisterCallback
        self.assertEqual(self._vts_tcp_server.
                         RegisterCallback(func_id, callback_func), True)

        # Connect to server
        response_message = self.ConnectToServer(func_id)

        # Confirm whether the callback_func() was not called.
        self.assertEqual(self.counter, prev_value + 1)

        # also confirm the error message
        self.assertEqual(response_message.response_code, SysMsg_pb2.SUCCESS)

        # Now unregister the function and check again
        # Function should be unregistered with UnegisterCallback
        # and the key should also be present
        self.assertTrue(self._vts_tcp_server.UnregisterCallback(func_id))

        # Capture the previous value of global counter
        prev_value = self.counter

        # Connect to server
        response_message = self.ConnectToServer(func_id)

        # Confirm whether the callback_func() was not called.
        self.assertEqual(self.counter, prev_value)

        # also confirm the error message
        self.assertEqual(response_message.response_code, SysMsg_pb2.FAIL)

if __name__ == '__main__':
    unittest.main()

