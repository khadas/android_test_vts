#
# Copyright (C) 2018 The Android Open Source Project
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

from vts.proto import AndroidSystemControlMessage_pb2 as ASysCtrlMsg
from vts.proto import VtsResourceControllerMessage_pb2 as ResControlMsg
from vts.proto import ComponentSpecificationMessage_pb2 as CompSpecMsg
from vts.utils.python.mirror import mirror_object


class ResourceMirror(mirror_object.MirrorObject):
    """This is a class that acts as a mirror object that represents resource
       allocated on the target side.

    Attributes:
        _client: the TCP client instance.
        _driver_id: int, used to identify the driver on the target side.
        _res_type: string, identifies the resource type of the current mirror
          (e.g. "fmq", "hidl_memory", "hidl_handle")
        _data_type: type of data in the queue.
        _sync: bool, whether the queue is synchronized
    """

    def __init__(self, client):
        super(ResourceMirror, self).__init__(client)

    def initFmq(self, data_type, sync, queue_id, queue_size, blocking,
                reset_pointers):
        """Initiate a fast message queue object on the target side,
           and stores the queue_id in the class attribute.

        Args:
            data_type: string, type of data in the queue (e.g. "uint32_t", "int16_t").
            sync: bool, whether queue is synchronized (only has one reader).
            queue_id: int, identifies the message queue object on the target side.
            queue_size: int, size of the queue.
            blocking: bool, whether blocking is enabled in the queue.
            reset_pointers: bool, whether to reset read/write pointers when
              creating a message queue object based on an existing message queue.
        """
        # Sets some configuration for the current resource mirror.
        self._res_type = "fmq"
        self._data_type = data_type
        self._sync = sync

        # Prepare arguments.
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_CREATE, queue_id)
        request_msg.queue_size = queue_size
        request_msg.blocking = blocking
        request_msg.reset_pointers = reset_pointers

        # Send and receive data.
        fmq_response = self._client.SendFmqRequest(request_msg)
        if (fmq_response != None):
            self._driver_id = fmq_response.queue_id
        else:
            self._driver_id = -1

    def read(self, data, data_size):
        """Initiate a non-blocking read request to FMQ driver.

        Args:
            data: list, data to be filled by this function. The list will
                  be emptied before the function starts to put read data into
                  it, which is consistent with the function behavior on the
                  target side.
            data_size: int, length of data to read.

        Returns:
            bool, true if the operation succeeds,
                  false otherwise.
        """
        # Prepare arguments.
        del data[:]
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_READ, self._driver_id)
        request_msg.read_data_size = data_size

        # Send and receive data.
        fmq_response = self._client.SendFmqRequest(request_msg)
        if fmq_response is not None and fmq_response.success:
            self.extractReadData(fmq_response, data)
            return True
        return False

    # TODO: support long-form blocking read in the future when there is use case.
    def readBlocking(self, data, data_size, time_out_nanos=0):
        """Initiate a blocking read request (short-form) to FMQ driver.

        Args:
            data: list, data to be filled by this function. The list will
                  be emptied before the function starts to put read data into
                  it, which is consistent with the function behavior on the
                  target side.
            data_size: int, length of data to read.
            time_out_nanos: int, wait time (in nanoseconds) when blocking.
                            The default value is 0 (no blocking).

        Returns:
            bool, true if the operation succeeds,
                  false otherwise.
        """
        # Prepare arguments.
        del data[:]
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_READ_BLOCKING, self._driver_id)
        request_msg.read_data_size = data_size
        request_msg.time_out_nanos = time_out_nanos

        # Send and receive data.
        fmq_response = self._client.SendFmqRequest(request_msg)
        if fmq_response is not None and fmq_response.success:
            self.extractReadData(fmq_response, data)
            return True
        return False

    def write(self, data, data_size):
        """Initiate a non-blocking write request to FMQ driver.

        Args:
            data: list, data to be written.
            data_size: int, length of data to write.
                       The function will only write data up until data_size,
                       i.e. extraneous data will be discarded.

        Returns:
            bool, true if the operation succeeds,
                  false otherwise.
        """
        # Prepare arguments.
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_WRITE, self._driver_id)
        self.prepareWriteData(request_msg, data[:data_size])

        # Send and receive data.
        fmq_response = self._client.SendFmqRequest(request_msg)
        if (fmq_response != None):
            return fmq_response.success
        return False

    # TODO: support long-form blocking write in the future when there is use case.
    def writeBlocking(self, data, data_size, time_out_nanos=0):
        """Initiate a blocking write request (short-form) to FMQ driver.

        Args:
            data: list, data to be written.
            data_size: int, length of data to write.
                       The function will only write data up until data_size,
                       i.e. extraneous data will be discarded.
            time_out_nanos: int, wait time (in nanoseconds) when blocking.
                            The default value is 0 (no blocking).

        Returns:
            bool, true if the operation succeeds,
                  false otherwise.
        """
        # Prepare arguments.
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_WRITE_BLOCKING, self._driver_id)
        self.prepareWriteData(request_msg, data[:data_size])
        request_msg.time_out_nanos = time_out_nanos

        # Send and receive data.
        fmq_response = self._client.SendFmqRequest(request_msg)
        if (fmq_response != None):
            return fmq_response.success
        return False

    def availableToWrite(self):
        """Gets space available to write in the queue.

        Returns:
            int, number of slots available.
        """
        # Prepare arguments.
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_AVAILABLE_WRITE, self._driver_id)

        # Send and receive data.
        return self.processUtilMethod(request_msg)

    def availableToRead(self):
        """Gets number of items available to read.

        Returns:
            int, number of items.
        """
        # Prepare arguments.
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_AVAILABLE_READ, self._driver_id)

        # Send and receive data.
        return self.processUtilMethod(request_msg)

    def getQuantumSize(self):
        """Gets size of item in the queue.

        Returns:
            int, size of item.
        """
        # Prepare arguments.
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_GET_QUANTUM_SIZE, self._driver_id)

        # send and receive data
        return self.processUtilMethod(request_msg)

    def getQuantumCount(self):
        """Gets number of items that fit in the queue.

        Returns:
            int, number of items.
        """
        # Prepare arguments.
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_GET_QUANTUM_COUNT, self._driver_id)

        # Send and receive data.
        return self.processUtilMethod(request_msg)

    def isValid(self):
        """Check if the queue is valid.

        Returns:
            bool, true if the queue is valid.
        """
        # Prepare arguments.
        request_msg = self.createTemplateFmqRequestMessage(
            ResControlMsg.FMQ_PROTO_IS_VALID, self._driver_id)

        # Send and receive data.
        fmq_response = self._client.SendFmqRequest(request_msg)
        if (fmq_response != None):
            return fmq_response.success
        return False

    def getQueueId(self):
        """Gets the id assigned from the target side.

        Returns:
            int, id of the queue.
        """
        return self._driver_id

    def createTemplateFmqRequestMessage(self, operation, queue_id):
        """Creates a template FmqRequestMessage with common arguments among
           all FMQ operations.

        Args:
            operation: FmqOp, fmq operations.
                       (see test/vts/proto/VtsResourceControllerMessage.proto).
            queue_id: int, identifies the message queue object on target side.

        Returns:
            FmqRequestMessage, fmq request message.
                (See test/vts/proto/VtsResourceControllerMessage.proto).
        """
        request_msg = ResControlMsg.FmqRequestMessage()
        request_msg.operation = operation
        request_msg.data_type = self._data_type
        request_msg.sync = self._sync
        request_msg.queue_id = queue_id
        return request_msg

    # Converts a python list into protobuf message.
    # TODO: Consider use py2pb once we know how to support user-defined types.
    #       This method only supports primitive types like int32_t, bool_t.
    def prepareWriteData(self, request_msg, data):
        """Prepares write data by converting python list into
           repeated protobuf field.

        Args:
            request_msg: FmqRequestMessage, arguments for a FMQ operation request.
            data: list, list of data items.
        """
        for curr_value in data:
            new_message = request_msg.write_data.add()
            new_message.type = CompSpecMsg.TYPE_SCALAR
            new_message.scalar_type = self._data_type
            setattr(new_message.scalar_value, self._data_type, curr_value)

    def extractReadData(self, response_msg, data):
        """Extracts read data from the response message returned by client.

        Args:
            response_msg: FmqResponseMessage, contains response from FMQ driver.
            data: list, to be filled by this function. data buffer is provided
                  by caller, so this function will append every element to the
                  buffer.
        """
        for item in response_msg.read_data:
            data.append(self._client.GetPythonDataOfVariableSpecMsg(item))

    def processUtilMethod(self, request_msg):
        """Sends request message and process response message for util methods
           that return an unsigned integer,
           e.g. availableToWrite, availableToRead.

        Args:
            request_msg: FmqRequestMessage, arguments for a FMQ operation request.

        Returns: int, information about the queue,
                 None if the operation is unsuccessful.
        """
        fmq_response = self._client.SendFmqRequest(request_msg)
        if (fmq_response != None and fmq_response.success):
            return fmq_response.sizet_return_val
        return None
