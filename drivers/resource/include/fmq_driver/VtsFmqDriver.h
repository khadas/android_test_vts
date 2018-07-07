//
// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef __VTS_RESOURCE_VTSFMQDRIVER_H
#define __VTS_RESOURCE_VTSFMQDRIVER_H

#include <mutex>
#include <string>
#include <typeinfo>
#include <unordered_map>

#include <android-base/logging.h>
#include <fmq/MessageQueue.h>

using android::hardware::kSynchronizedReadWrite;
using android::hardware::kUnsynchronizedWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::MQDescriptorUnsync;

using namespace std;
using QueueId = int;

namespace android {
namespace vts {

// struct to store queue information.
struct QueueInfo {
  // type of data in the queue.
  const type_info& queue_data_type;
  // flavor of the queue (sync or unsync).
  hardware::MQFlavor queue_flavor;
  // pointer to the actual queue object.
  shared_ptr<void> queue_object;
};

// possible operation on the queue.
enum FmqOperation {
  // unknown operation.
  FMQ_OP_UNKNOWN,
  // create a new queue, or new object based on an existing queue.
  FMQ_OP_CREATE,
  // read without blocking.
  FMQ_OP_READ,
  // read with blocking (short form, no multiple queue blocking).
  FMQ_OP_READ_BLOCKING,
  // read with blocking (long form, possible multiple queue blocking).
  // target side only for now.
  FMQ_OP_READ_BLOCKING_LONG,
  // write without blocking.
  FMQ_OP_WRITE,
  // write with blocking (short form, no multiple queue blocking).
  FMQ_OP_WRITE_BLOCKING,
  // write with blocking (long form, possible multiple queue blocking).
  // target side only for now.
  FMQ_OP_WRITE_BLOCKING_LONG,
  // gets space available to write.
  FMQ_OP_AVAILABLE_WRITE,
  // gets number of items available to read.
  FMQ_OP_AVAILABLE_READ,
  // gets size of item.
  FMQ_OP_GET_QUANTUM_SIZE,
  // gets number of items that fit into the queue.
  FMQ_OP_GET_QUANTUM_COUNT,
  // checks if the queue is valid.
  FMQ_OP_IS_VALID,
  // gets event flag word, which allows multiple queue blocking.
  FMQ_OP_GET_EVENT_FLAG_WORD
};

// struct to store parameters in an operation.
struct OperationParam {
  OperationParam()
      : op(FMQ_OP_UNKNOWN),
        queue_id(-1),
        queue_size(0),
        blocking(false),
        reset_pointers(true),
        data_size(0),
        read_notification(0),
        write_notification(0),
        time_out_nanos(0),
        event_flag_word(nullptr){};
  FmqOperation op;      // operation to perform on FMQ.
  QueueId queue_id;     // identifies the message queue object.
  size_t queue_size;    // size of the queue.
  bool blocking;        // whether to enable blocking in the queue.
  bool reset_pointers;  // whether to reset read/write pointers when creating a
                        // message queue object based on an existing message
                        // queue.
  size_t data_size;     // length of the data.
  uint32_t read_notification;         // bits to set when finish reading.
  uint32_t write_notification;        // bits to set when finish writing.
  int64_t time_out_nanos;             // wait time while blocking.
  atomic<uint32_t>* event_flag_word;  // allows for multiple queue blocking.
};

// different operations on the queue have different return value types.
union RequestResult {
  // stores results from functions that return int (create).
  int int_val;
  // stores results from functions that return size_t (e.g. GetQuantumSize).
  size_t sizet_val;
  // stores results from functions that return pointer (e.g. GetEventFlagWord).
  atomic<uint32_t>* pointer_val;
};

// A fast message queue class that manages all fast message queues created
// on the target side. Reader and writer use their id to read from and write
// into the queue.
// Example:
//   VtsFmqDriver manager;
//   // creates one reader and one writer.
//   QueueId writer_id = manager.CreateFmq("uint16_t", true, NUM_ELEMS, false);
//   QueueId reader_id = manager.CreateFmq("uint16_t", true, writer_id);
//   // write some data
//   uint16_t write_data[5] {1, 2, 3, 4, 5};
//   manager.WriteFmq("uint16_t", true, writer_id,
//                    static_cast<void*>(write_data), 5);
//   // read the same data back
//   uint16_t read_data[5];
//   manager.ReadFmq("uint16_t", true, reader_id,
//                   static_cast<void*>(read_data), 5);
class VtsFmqDriver {
 public:
  // Constructor to initialize a Fast Message Queue (FMQ) manager.
  VtsFmqDriver();

  // Virtual destructor to clean up the class.
  ~VtsFmqDriver();

  // Creates a brand new FMQ, i.e. the "first message queue object".
  //
  // @param type       type of data in the queue.
  // @param sync       whether queue is synchronized (only has one reader).
  // @param queue_size number of elements in the queue.
  // @param blocking   whether to enable blocking within the queue.
  //
  // @return message queue object id associated with the caller on success,
  //         -1 on failure.
  QueueId CreateFmq(string type, bool sync, size_t queue_size, bool blocking);

  // Creates a new FMQ object based on an existing message queue.
  //
  // @param type           string, queue data type.
  // @param sync           whether queue is synchronized (only has one reader).
  // @param queue_id       identifies the message queue object.
  // @param reset_pointers whether to reset read and write pointers.
  //
  // @return message queue object id associated with the caller on success,
  //         -1 on failure.
  QueueId CreateFmq(string type, bool sync, QueueId queue_id,
                    bool reset_pointers = true);

  // Reads one item from FMQ (no blocking at all).
  //
  // @param type     type of data in the queue data type.
  // @param sync     whether queue is synchronized (only has one reader).
  // @param queue_id identifies the message queue object.
  // @param data     pointer to the start of data to be filled.
  //
  // @return true if no error happens when reading from FMQ,
  //         false otherwise.
  bool ReadFmq(string type, bool sync, QueueId queue_id, void* data);

  // Reads data_size items from FMQ (no blocking at all).
  //
  // @param type      type of data in the queue.
  // @param sync      whether queue is synchronized (only has one reader).
  // @param queue_id  identifies the message queue object.
  // @param data      pointer to the start of data to be filled.
  // @param data_size number of items to read.
  //
  // @return true if no error happens when reading from FMQ,
  //         false otherwise.
  bool ReadFmq(string type, bool sync, QueueId queue_id, void* data,
               size_t data_size);

  // Reads data_size items from FMQ, block if there is not enough data to
  // read.
  // This method can only be called if blocking=true on creation of the "first
  // message queue object" of the FMQ.
  //
  // @param type           type of data in the queue.
  // @param sync           whether queue is synchronized (only has one reader).
  // @param queue_id       identifies the message queue object.
  // @param data           pointer to the start of data to be filled.
  // @param data_size      number of items to read.
  // @param time_out_nanos wait time when blocking.
  //
  // Returns: true if no error happens when reading from FMQ,
  //          false otherwise.
  bool ReadFmqBlocking(string type, bool sync, QueueId queue_id, void* data,
                       size_t data_size, int64_t time_out_nanos);

  // Reads data_size items from FMQ, possibly block on other queues.
  //
  // @param type               type of data in the queue.
  // @param sync               whether queue is synchronized.
  // @param queue_id           identifies the message queue object.
  // @param data               pointer to the start of data to be filled.
  // @param data_size          number of items to read.
  // @param read_notification  notification bits to set when finish reading.
  // @param write_notification notification bits to wait on when blocking.
  //                           Read will fail if this argument is 0.
  // @param time_out_nanos     wait time when blocking.
  // @param event_flag_word    event flag word shared by multiple queues.
  //
  // @return true if no error happens when reading from FMQ,
  //         false otherwise.
  bool ReadFmqBlocking(string type, bool sync, QueueId queue_id, void* data,
                       size_t data_size, uint32_t read_notification,
                       uint32_t write_notification, int64_t time_out_nanos,
                       atomic<uint32_t>* event_flag_word);

  // Writes one item to FMQ (no blocking at all).
  //
  // @param type     type of data in the queue.
  // @param sync     whether queue is synchronized (only has one reader).
  // @param queue_id identifies the message queue object.
  // @param data     pointer to the start of data to be written.
  //
  // @return true if no error happens when writing to FMQ,
  //         false otherwise.
  bool WriteFmq(string type, bool sync, QueueId queue_id, void* data);

  // Writes data_size items to FMQ (no blocking at all).
  //
  // @param type      type of data in the queue.
  // @param sync      whether queue is synchronized (only has one reader).
  // @param queue_id  identifies the message queue object.
  // @param data      pointer to the start of data to be written.
  // @param data_size number of items to write.
  //
  // @return true if no error happens when writing to FMQ,
  //         false otherwise.
  bool WriteFmq(string type, bool sync, QueueId queue_id, void* data,
                size_t data_size);

  // Writes data_size items to FMQ, block if there is not enough space in
  // the queue.
  // This method can only be called if blocking=true on creation of the "first
  // message queue object" of the FMQ.
  //
  // @param type           type of data in the queue.
  // @param sync           whether queue is synchronized (only has one reader).
  // @param queue_id       identifies the message queue object.
  // @param data           pointer to the start of data to be written.
  // @param data_size      number of items to write.
  // @param time_out_nanos wait time when blocking.
  //
  // @returns true if no error happens when writing to FMQ,
  //          false otherwise
  bool WriteFmqBlocking(string type, bool sync, QueueId queue_id, void* data,
                        size_t data_size, int64_t time_out_nanos);

  // Writes data_size items to FMQ, possibly block on other queues.
  //
  // @param type               type of data in the queue.
  // @param sync               whether queue is synchronized.
  // @param queue_id           identifies the message queue object.
  // @param data               pointer to the start of data to be written.
  // @param data_size          number of items to write.
  // @param read_notification  notification bits to wait on when blocking.
  //                           Write will fail if this argument is 0.
  // @param write_notification notification bits to set when finish writing.
  // @param time_out_nanos     wait time when blocking.
  // @param event_flag_word    event flag word shared by multiple queues.
  //
  // @return true if no error happens when writing to FMQ,
  //         false otherwise.
  bool WriteFmqBlocking(string type, bool sync, QueueId queue_id, void* data,
                        size_t data_size, uint32_t read_notification,
                        uint32_t write_notification, int64_t time_out_nanos,
                        atomic<uint32_t>* event_flag_word);

  // Gets space available to write in the queue.
  //
  // @param type     type of data in the queue.
  // @param sync     whether queue is synchronized (only has one reader).
  // @param queue_id identifies the message queue object.
  // @param result   pointer to the result. Use pointer to store result because
  //                 the return value signals if the queue is found correctly.
  //
  // @return true if queue is found and type matches, and puts actual result in
  //              result pointer,
  //         false otherwise.
  bool AvailableToWrite(string type, bool sync, QueueId queue_id,
                        size_t* result);

  // Gets number of items available to read in the queue.
  //
  // @param type     type of data in the queue.
  // @param sync     whether queue is synchronized (only has one reader).
  // @param queue_id identifies the message queue object.
  // @param result   pointer to the result. Use pointer to store result because
  //                 the return value signals if the queue is found correctly.
  //
  // @return true if queue is found and type matches, and puts actual result in
  //              result pointer,
  //         false otherwise.
  bool AvailableToRead(string type, bool sync, QueueId queue_id,
                       size_t* result);

  // Gets size of item in the queue.
  //
  // @param type     type of data in the queue.
  // @param sync     whether queue is synchronized (only has one reader).
  // @param queue_id identifies the message queue object.
  // @param result   pointer to the result. Use pointer to store result because
  //                 the return value signals if the queue is found correctly.
  //
  // @return true if queue is found and type matches, and puts actual result in
  //              result pointer,
  //         false otherwise.
  bool GetQuantumSize(string type, bool sync, QueueId queue_id, size_t* result);

  // Gets number of items that fit in the queue.
  //
  // @param type     type of data in the queue.
  // @param sync     whether queue is synchronized (only has one reader).
  // @param queue_id identifies the message queue object.
  // @param result   pointer to the result. Use pointer to store result because
  //                 the return value signals if the queue is found correctly.
  //
  // @return true if queue is found and type matches, and puts actual result in
  //              result pointer,
  //         false otherwise.
  bool GetQuantumCount(string type, bool sync, QueueId queue_id,
                       size_t* result);

  // Checks if the queue associated with queue_id is valid.
  //
  // @param type     type of data in the queue.
  // @param sync     whether queue is synchronized (only has one reader).
  // @param queue_id identifies the message queue object.
  //
  // @return true if the queue object is valid, false otherwise.
  bool IsValid(string type, bool sync, QueueId queue_id);

  // Gets event flag word of the queue, which allows multiple queues
  // to communicate (i.e. blocking).
  // The returned event flag word can be passed into readBlocking() and
  // writeBlocking() to achieve blocking among multiple queues.
  //
  // @param type     type of data in the queue.
  // @param sync     whether queue is synchronized (only has one reader).
  // @param queue_id identifies the message queue object.
  // @param result   pointer to the result. Use pointer to store result because
  //                 the return value signals if the queue is found correctly.
  //
  // @return true if queue is found and type matches, and puts actual result in
  //              result pointer,
  //         false otherwise.
  bool GetEventFlagWord(string type, bool sync, QueueId queue_id,
                        atomic<uint32_t>** result);

 private:
  // Finds the queue in the map based on the input queue ID.
  //
  // @param queue_id identifies the queue object.
  //
  // @return the pointer to message queue object,
  //         nullptr if queue ID is invalid or queue type is misspecified.
  template <typename T, hardware::MQFlavor flavor>
  MessageQueue<T, flavor>* FindQueue(QueueId queue_id);

  // Processes the request from the host.
  //
  // @param type     type of data in the queue.
  // @param sync     whether the queue is synchronized (only has one reader).
  // @param op_param reference to a struct that stores parameters in the
  //                 operation request.
  // @param result   pointer to the result, since various functions have
  //                 different types of return values.
  // @param data     pointer to the start of data buffer.
  //
  // @return true if queue is found and type checking passes, and the
  //              actual result is stored in result pointer,
  //         false otherwise.
  bool ProcessRequest(string type, bool sync, const OperationParam& op_param,
                      RequestResult* result, void* data = nullptr);

  // Helper method to determine the appropriate function to call and
  // template to provide when performing operation on the queue, depending on
  // whether the queue is synchronized, and whether the operation involves
  // blocking read/write.
  //
  // @param sync        whether the queue is synchronized.
  // @param op_param    reference to a struct that stores parameters in the
  //                    operation request.
  // @param result      pointer to the result, since various functions have
  //                    different types of return values.
  // @param data        pointer to the start of data buffer.
  // @param is_blocking whether blocking is enabled in the queue.
  //
  // @return true if queue is found and type checking passes, and the
  //              actual result is stored in result struct,
  //         false otherwise.
  template <typename T>
  bool ExecuteOperationHelper(bool sync, const OperationParam& op_param,
                              RequestResult* result, T* data, bool is_blocking);

  // Executes the operation specified by the caller (non-blocking operation
  // only).
  //
  // @param op_param reference to a struct that stores parameters in the
  //                 operation request.
  // @param result   pointer to the result, since various functions have
  //                 different types of return values.
  // @param data     pointer to the start of data buffer.
  //
  // @return true if queue is found and type checking passes, and the
  //              actual result is stored in result struct,
  //         false otherwise.
  template <typename T, hardware::MQFlavor>
  bool ExecuteNonBlockingOperation(const OperationParam& op_param,
                                   RequestResult* result, T* data);

  // Executes the operation specified by the caller (blocking operation).
  //
  // @param op_param reference to a struct that stores parameters in the
  //                 operation request.
  // @param data     pointer to the start of data buffer.
  //
  // @return true if the operation succeeds,
  //         false otherwise.
  template <typename T>
  bool ExecuteBlockingOperation(const OperationParam& op_param, T* data);

  // Inserts QueueInfo object into fmq_map_, while ensuring thread safety on
  // fmq_map_.
  //
  // @param queue_info QueueInfo object.
  //
  // @return id associated with the queue.
  QueueId InsertQueue(unique_ptr<QueueInfo> queue_info);

  // Processes util methods that return size_t (AvailableToWrite,
  // AvailableToRead, GetQuantumCount, GetQuantumSize).
  //
  // @param type     type of data in the queue.
  // @param sync     whether the queue is synchronized (only has one reader).
  // @param op       operation on the queue.
  // @param queue_id identifies the message queue object.
  // @param result   pointer that stores result.
  //
  // @return true if queue is found and type checking passes, and stores the
  //              result in result pointer,
  //         false otherwise.
  bool ProcessUtilMethod(string type, bool sync, FmqOperation op,
                         QueueId queue_id, size_t* result);

  // a hashtable to keep track of all ongoing FMQ's.
  // The key of the hashtable is the queue ID.
  // The value of the hashtable is a smart pointer to message queue object
  // information struct associated with the queue ID.
  unordered_map<QueueId, unique_ptr<QueueInfo>> fmq_map_;

  // a mutex to ensure mutual exclusion of operations on fmq_map_
  mutex map_mutex_;
};

}  // namespace vts
}  // namespace android
#endif  //__VTS_RESOURCE_VTSFMQDRIVER_H
