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
class VtsFmqDriver {
 public:
  // Constructor to initialize a Fast Message Queue (FMQ) manager.
  VtsFmqDriver();

  // Virtual destructor to clean up the class.
  ~VtsFmqDriver();

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
