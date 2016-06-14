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



class TCPRequestHandler(SocketServer.BaseRequestHandler):
    """The request handler class for our server."""

    def handle(self):
        """Receives requests from clients.

        This function receives the request from the client and sends the text
        by converting them to upper case characters.
        """
        # self.request is the TCP socket connected to the client
        data = self.request.recv(1024).strip()
        logging.info('client address: ', self.client_address[0])
        logging.info('data: ', data)

        cur_thread = threading.current_thread()
        response = "{}: {}".format(cur_thread.name, data.upper())

        # just send back the same data, but upper-cased
        self.request.sendall(response)


class ThreadedTCPServer(object):
    """This class creates TCPServer in separate thread.

    Attributes:
        _server:   an instance of SocketServer.TCPServer.
        _port_used: this variable maintains the port number used in creating
                  the server connection.
        _IP_address: variable to hold the IP Address of the host.

    """
    _server = None
    _port_used = 0  # Port 0 means to select an arbitrary unused port
    _IP_address = ""  # Used to store the IP address for the server
    _HOST = "localhost"  # IP address to which initial connection is made

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
