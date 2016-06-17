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

import SocketServer
import logging
import threading
from socket import error as socket_error
from vts.runners.host.errors import TcpServerCreationError
from vts.runners.host.errors import TcpServerShutdownError

_functions = dict()  # Dictionary to hold function pointers

class TCPRequestHandler(SocketServer.BaseRequestHandler):
    """The request handler class for our server.

    Attributes:
        _error_reponse: The response message sent to client when the function
            is not registered in the dictionary.
        _success_response: Message sent when the function is registered with
            the dictionary.
    """

    _error_response = "Error: Function not registered."
    _success_response = "Success"

    def handle(self):
        """Receives requests from clients.

        This function receives the request from the client and sends the text
        by converting them to upper case characters.
        """
        # self.request is the TCP socket connected to the client
        data = self.request.recv(1024).strip()

        logging.info('client address: ', self.client_address[0])
        logging.info('data: ', data)
        logging.info('Current Thread: ', threading.current_thread())

        if data in _functions:
            _functions[data]()  # call the function pointer
            response = self._success_response
        else:
            response = self._error_response

        self.request.sendall(response)  # send the response back to client


class ThreadedTCPServer(object):
    """This class creates TCPServer in separate thread.

    Attributes:
        _server:   an instance of SocketServer.TCPServer.
        _port_used: this variable maintains the port number used in creating
                  the server connection.
        _IP_address: variable to hold the IP Address of the host.
        _HOST: IP Address to which initial connection is made.
    """
    _server = None
    _port_used = 0  # Port 0 means to select an arbitrary unused port
    _IP_address = ""  # Used to store the IP address for the server
    _HOST = "localhost"  # IP address to which initial connection is made

    def RegisterCallback(self, func_id, callback_func):
        """To register the function pointer.

        This method registers the callback for the _functions by storing the
        function pointer in set.

        Args:
            func_id: Refers to the func_id of function pointers that is maintained in
                the dictionary.
            callback_func:  Refers to the callback_func that we need to
                            register.

        Returns:
            False if func_id is already present in the dictionary, func_id is
            None or callback_func is None; else returns True after inserting
            the key-func_id and value-callback_func.
        """

        if func_id is None or func_id in _functions or callback_func is None:
            return False
        else:
            _functions[func_id] = callback_func  # update the dictionary
            return True

    def UnregisterCallback(self, func_id):
        """Removes the function pointer from the dict corresponding to the key.

        Args:
            func_id: The function id against which the function pointer
                is stored.

        Returns:
            Returns false if func_id is not present in the dictionary -
                _functions, else return True after removing it from dict.
        """

        if func_id is None or func_id not in _functions:
            return False
        else:
            _functions.pop(func_id, None)  #  Remove the key from dictionary
            return True

    def Start(self, port=0):
        """This function starts the TCP server.

        Args:
            port: The port at which connection will be made. Default value
                  is zero, in which case a free port will be chosen
                  automatically.

        Raises:
            TcpServerCreationError: Error occurred while starting server.
        """
        # Start the sever
        try:
            self._server = SocketServer.TCPServer((self._HOST, port),
                                                  TCPRequestHandler)
            self._IP_address, self._port_used = self._server.server_address

            # Start a thread with the server -- that thread will then start one
            # more thread for each request
            server_thread = threading.Thread(target = self._server.serve_forever)

            # Exit the server thread when the main thread terminates
            server_thread.daemon = True
            server_thread.start()
            logging.info('Server loop running in thread: %s', server_thread.name)

        except (RuntimeError, IOError, socket_error) as e:
            logging.exception(e)
            raise TcpServerCreationError('TcpServerCreationError occurred.')

    def Stop(self):
        """This function calls stop server to stop the server instance."""

        # stop the server instance
        self._server.shutdown
        self._server.server_close()

    def GetIPAddress(self):
        """Returns the IP Address used in creating the server."""
        return self._IP_address

    def GetPortUsed(self):
        """Returns the port number used in creating the server."""
        return self._port_used
