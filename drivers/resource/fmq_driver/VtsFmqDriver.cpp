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
#define LOG_TAG "VtsFmqDriver"

#include "fmq_driver/VtsFmqDriver.h"

#include <string>
#include <typeinfo>

#include <android-base/logging.h>
#include <fmq/MessageQueue.h>

using android::hardware::kSynchronizedReadWrite;
using android::hardware::kUnsynchronizedWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::MQDescriptorUnsync;

using namespace std;

namespace android {
namespace vts {

VtsFmqDriver::VtsFmqDriver() {}

VtsFmqDriver::~VtsFmqDriver() {
  // clears objects in the map.
  fmq_map_.clear();
}

QueueId VtsFmqDriver::CreateFmq(string type, bool sync, size_t queue_size,
                                bool blocking) {
  OperationParam params;
  params.op = FMQ_OP_CREATE;
  params.queue_size = queue_size;
  params.blocking = blocking;
  RequestResult result;

  bool success = ProcessRequest(type, sync, params, &result);
  if (success) return result.int_val;
  return -1;
}

QueueId VtsFmqDriver::CreateFmq(string type, bool sync, QueueId queue_id,
                                bool reset_pointers) {
  OperationParam params;
  params.op = FMQ_OP_CREATE;
  params.queue_id = queue_id;
  params.reset_pointers = reset_pointers;
  RequestResult result;

  bool success = ProcessRequest(type, sync, params, &result);
  if (success) return result.int_val;
  return -1;
}

bool VtsFmqDriver::ReadFmq(string type, bool sync, QueueId queue_id,
                           void* data) {
  return ReadFmq(type, sync, queue_id, data, 1);
}

bool VtsFmqDriver::ReadFmq(string type, bool sync, QueueId queue_id, void* data,
                           size_t data_size) {
  OperationParam params;
  params.op = FMQ_OP_READ;
  params.queue_id = queue_id;
  params.data_size = data_size;
  RequestResult result;

  return ProcessRequest(type, sync, params, &result, data);
}

bool VtsFmqDriver::ReadFmqBlocking(string type, bool sync, QueueId queue_id,
                                   void* data, size_t data_size,
                                   int64_t time_out_nanos) {
  OperationParam params;
  params.op = FMQ_OP_READ_BLOCKING;
  params.queue_id = queue_id;
  params.data_size = data_size;
  params.time_out_nanos = time_out_nanos;
  RequestResult result;

  return ProcessRequest(type, sync, params, &result, data);
}

bool VtsFmqDriver::ReadFmqBlocking(string type, bool sync, QueueId queue_id,
                                   void* data, size_t data_size,
                                   uint32_t read_notification,
                                   uint32_t write_notification,
                                   int64_t time_out_nanos,
                                   atomic<uint32_t>* event_flag_word) {
  OperationParam params;
  params.op = FMQ_OP_READ_BLOCKING_LONG;
  params.queue_id = queue_id;
  params.data_size = data_size;
  params.read_notification = read_notification;
  params.write_notification = write_notification;
  params.time_out_nanos = time_out_nanos;
  params.event_flag_word = event_flag_word;
  RequestResult result;

  return ProcessRequest(type, sync, params, &result, data);
}

bool VtsFmqDriver::WriteFmq(string type, bool sync, QueueId queue_id,
                            void* data) {
  return WriteFmq(type, sync, queue_id, data, 1);
}

bool VtsFmqDriver::WriteFmq(string type, bool sync, QueueId queue_id,
                            void* data, size_t data_size) {
  OperationParam params;
  params.op = FMQ_OP_WRITE;
  params.queue_id = queue_id;
  params.data_size = data_size;
  RequestResult result;

  return ProcessRequest(type, sync, params, &result, data);
}

bool VtsFmqDriver::WriteFmqBlocking(string type, bool sync, QueueId queue_id,
                                    void* data, size_t data_size,
                                    int64_t time_out_nanos) {
  OperationParam params;
  params.op = FMQ_OP_WRITE_BLOCKING;
  params.queue_id = queue_id;
  params.data_size = data_size;
  params.time_out_nanos = time_out_nanos;
  RequestResult result;

  return ProcessRequest(type, sync, params, &result, data);
}

bool VtsFmqDriver::WriteFmqBlocking(string type, bool sync, QueueId queue_id,
                                    void* data, size_t data_size,
                                    uint32_t read_notification,
                                    uint32_t write_notification,
                                    int64_t time_out_nanos,
                                    atomic<uint32_t>* event_flag_word) {
  OperationParam params;
  params.op = FMQ_OP_WRITE_BLOCKING_LONG;
  params.queue_id = queue_id;
  params.data_size = data_size;
  params.read_notification = read_notification;
  params.write_notification = write_notification;
  params.time_out_nanos = time_out_nanos;
  params.event_flag_word = event_flag_word;
  RequestResult result;

  return ProcessRequest(type, sync, params, &result, data);
}

bool VtsFmqDriver::AvailableToWrite(string type, bool sync, QueueId queue_id,
                                    size_t* result) {
  return ProcessUtilMethod(type, sync, FMQ_OP_AVAILABLE_WRITE, queue_id,
                           result);
}

bool VtsFmqDriver::AvailableToRead(string type, bool sync, QueueId queue_id,
                                   size_t* result) {
  return ProcessUtilMethod(type, sync, FMQ_OP_AVAILABLE_READ, queue_id, result);
}

bool VtsFmqDriver::GetQuantumSize(string type, bool sync, QueueId queue_id,
                                  size_t* result) {
  return ProcessUtilMethod(type, sync, FMQ_OP_GET_QUANTUM_SIZE, queue_id,
                           result);
}

bool VtsFmqDriver::GetQuantumCount(string type, bool sync, QueueId queue_id,
                                   size_t* result) {
  return ProcessUtilMethod(type, sync, FMQ_OP_GET_QUANTUM_COUNT, queue_id,
                           result);
}

bool VtsFmqDriver::IsValid(string type, bool sync, QueueId queue_id) {
  OperationParam params;
  params.op = FMQ_OP_IS_VALID;
  params.queue_id = queue_id;
  RequestResult result;

  return ProcessRequest(type, sync, params, &result);
}

bool VtsFmqDriver::GetEventFlagWord(string type, bool sync, QueueId queue_id,
                                    atomic<uint32_t>** result) {
  OperationParam params;
  params.op = FMQ_OP_GET_EVENT_FLAG_WORD;
  params.queue_id = queue_id;
  RequestResult request_result;

  bool success = ProcessRequest(type, sync, params, &request_result);
  if (success) *result = request_result.pointer_val;  // store result in pointer
  return success;
}

template <typename T, hardware::MQFlavor flavor>
MessageQueue<T, flavor>* VtsFmqDriver::FindQueue(QueueId queue_id) {
  auto iterator = fmq_map_.find(queue_id);
  if (iterator == fmq_map_.end()) {  // queue not found
    LOG(ERROR) << "Cannot find Fast Message Queue with ID " << queue_id;
    return nullptr;
  }

  QueueInfo* queue_info = (iterator->second).get();
  if (queue_info->queue_data_type != typeid(T)) {  // queue data type incorrect
    LOG(ERROR)
        << "Caller specified data type doesn't match with queue data type.";
    return nullptr;
  }

  if (queue_info->queue_flavor != flavor) {  // queue flavor incorrect
    LOG(ERROR)
        << "Caller specified flavor doesn't match with the queue flavor.";
    return nullptr;
  }

  // type check passes, extract queue from the struct
  shared_ptr<MessageQueue<T, flavor>> queue_object =
      static_pointer_cast<MessageQueue<T, flavor>>(queue_info->queue_object);
  return queue_object.get();
}

bool VtsFmqDriver::ProcessRequest(string type, bool sync,
                                  const OperationParam& op_param,
                                  RequestResult* result, void* data) {
  bool is_blocking_operation = op_param.op == FMQ_OP_READ_BLOCKING ||
                               op_param.op == FMQ_OP_READ_BLOCKING_LONG ||
                               op_param.op == FMQ_OP_WRITE_BLOCKING ||
                               op_param.op == FMQ_OP_WRITE_BLOCKING_LONG;
  // blocking operation only supports synchronized queue
  if (is_blocking_operation && !sync) {
    LOG(ERROR) << "Unsynchronized queue doesn't support blocking read/write.";
    return false;
  }

  // infer type dynamically, TODO: support more types in the future
  if (!type.compare("uint64_t")) {
    return ExecuteOperationHelper<uint64_t>(sync, op_param, result,
                                            static_cast<uint64_t*>(data),
                                            is_blocking_operation);
  } else if (!type.compare("int64_t")) {
    return ExecuteOperationHelper<int64_t>(sync, op_param, result,
                                           static_cast<int64_t*>(data),
                                           is_blocking_operation);
  } else if (!type.compare("uint32_t")) {
    return ExecuteOperationHelper<uint32_t>(sync, op_param, result,
                                            static_cast<uint32_t*>(data),
                                            is_blocking_operation);
  } else if (!type.compare("int32_t")) {
    return ExecuteOperationHelper<int32_t>(sync, op_param, result,
                                           static_cast<int32_t*>(data),
                                           is_blocking_operation);
  } else if (!type.compare("uint16_t")) {
    return ExecuteOperationHelper<uint16_t>(sync, op_param, result,
                                            static_cast<uint16_t*>(data),
                                            is_blocking_operation);
  } else if (!type.compare("int16_t")) {
    return ExecuteOperationHelper<int16_t>(sync, op_param, result,
                                           static_cast<int16_t*>(data),
                                           is_blocking_operation);
  } else if (!type.compare("uint8_t")) {
    return ExecuteOperationHelper<uint8_t>(sync, op_param, result,
                                           static_cast<uint8_t*>(data),
                                           is_blocking_operation);
  } else if (!type.compare("int8_t")) {
    return ExecuteOperationHelper<int8_t>(sync, op_param, result,
                                          static_cast<int8_t*>(data),
                                          is_blocking_operation);
  } else if (!type.compare("float")) {
    return ExecuteOperationHelper<float>(sync, op_param, result,
                                         static_cast<float*>(data),
                                         is_blocking_operation);
  } else if (!type.compare("double")) {
    return ExecuteOperationHelper<double>(sync, op_param, result,
                                          static_cast<double*>(data),
                                          is_blocking_operation);
  } else if (!type.compare("bool")) {
    return ExecuteOperationHelper<bool>(sync, op_param, result,
                                        static_cast<bool*>(data),
                                        is_blocking_operation);
  }
  LOG(ERROR) << "Unknown type provided by caller.";
  return false;
}

template <typename T>
bool VtsFmqDriver::ExecuteOperationHelper(bool sync,
                                          const OperationParam& op_param,
                                          RequestResult* result, T* data,
                                          bool is_blocking) {
  if (is_blocking) {
    return ExecuteBlockingOperation<T>(op_param, data);
  } else {
    return sync ? ExecuteNonBlockingOperation<T, kSynchronizedReadWrite>(
                      op_param, result, data)
                : ExecuteNonBlockingOperation<T, kUnsynchronizedWrite>(
                      op_param, result, data);
  }
}

template <typename T, hardware::MQFlavor flavor>
bool VtsFmqDriver::ExecuteNonBlockingOperation(const OperationParam& op_param,
                                               RequestResult* result, T* data) {
  MessageQueue<T, flavor>* queue_object = nullptr;
  bool new_queue_request =
      (op_param.op == FMQ_OP_CREATE && op_param.queue_id == -1);
  // only find queue if it is not a create operation
  // otherwise, it will print queue not found to the error log
  if (!new_queue_request) {
    // need to lock around FindQueue because we make sure only one thread
    // accesses QueueInfo* at once.
    map_mutex_.lock();
    queue_object = FindQueue<T, flavor>(op_param.queue_id);
    map_mutex_.unlock();
    if (queue_object == nullptr) {
      return false;  // queue not found, or type mismatch
    }
  }

  switch (op_param.op) {
    case FMQ_OP_CREATE:
      if (op_param.queue_id == -1) {
        // creates a new queue, use smart pointers to avoid memory leak
        shared_ptr<MessageQueue<T, flavor>> new_queue(
            new (std::nothrow) MessageQueue<T, flavor>(op_param.queue_size,
                                                       op_param.blocking));
        unique_ptr<QueueInfo> new_queue_info(new QueueInfo{
            typeid(T), flavor, static_pointer_cast<void>(new_queue)});
        result->int_val = InsertQueue(move(new_queue_info));
      } else {  // create a queue object based on an existing queue
        const hardware::MQDescriptor<T, flavor>* descriptor =
            queue_object->getDesc();
        if (descriptor == nullptr) {
          LOG(ERROR)
              << "Cannot find descriptor for the specified Fast Message Queue";
          return false;  // likely not the first message queue object
        }

        shared_ptr<MessageQueue<T, flavor>> new_queue(
            new (std::nothrow)
                MessageQueue<T, flavor>(*descriptor, op_param.reset_pointers));
        unique_ptr<QueueInfo> new_queue_info(new QueueInfo{
            typeid(T), flavor, static_pointer_cast<void>(new_queue)});
        result->int_val = InsertQueue(move(new_queue_info));
      }
      break;
    case FMQ_OP_READ:
      return queue_object->read(data, op_param.data_size);
    case FMQ_OP_WRITE:
      return queue_object->write(data, op_param.data_size);
    case FMQ_OP_AVAILABLE_WRITE:
      result->sizet_val = queue_object->availableToWrite();
      break;
    case FMQ_OP_AVAILABLE_READ:
      result->sizet_val = queue_object->availableToRead();
      break;
    case FMQ_OP_GET_QUANTUM_SIZE:
      result->sizet_val = queue_object->getQuantumSize();
      break;
    case FMQ_OP_GET_QUANTUM_COUNT:
      result->sizet_val = queue_object->getQuantumCount();
      break;
    case FMQ_OP_IS_VALID:
      return queue_object->isValid();
    case FMQ_OP_GET_EVENT_FLAG_WORD:
      result->pointer_val = queue_object->getEventFlagWord();
      break;
    default:
      LOG(ERROR) << "unknown operation";
      return false;
  }

  return true;
}

template <typename T>
bool VtsFmqDriver::ExecuteBlockingOperation(const OperationParam& op_param,
                                            T* data) {
  // need to lock around FindQueue because we make sure only one thread accesses
  // QueueInfo* at once.
  map_mutex_.lock();
  MessageQueue<T, kSynchronizedReadWrite>* queue_object =
      FindQueue<T, kSynchronizedReadWrite>(op_param.queue_id);
  map_mutex_.unlock();
  if (queue_object == nullptr)
    return false;  // queue not found, or type mismatch

  hardware::EventFlag* ef_group = nullptr;
  status_t status;
  switch (op_param.op) {
    case FMQ_OP_READ_BLOCKING:
      return queue_object->readBlocking(data, op_param.data_size,
                                        op_param.time_out_nanos);
    case FMQ_OP_WRITE_BLOCKING:
      return queue_object->writeBlocking(data, op_param.data_size,
                                         op_param.time_out_nanos);
    case FMQ_OP_READ_BLOCKING_LONG:
      // create an event flag out of the event flag word
      status = hardware::EventFlag::createEventFlag(op_param.event_flag_word,
                                                    &ef_group);
      if (status != NO_ERROR) {  // check status
        LOG(ERROR) << "Creating event flag failure with code " << status;
        return false;
      }
      return queue_object->readBlocking(
          data, op_param.data_size, op_param.read_notification,
          op_param.write_notification, op_param.time_out_nanos, ef_group);
    case FMQ_OP_WRITE_BLOCKING_LONG:
      // create an event flag out of the event flag word
      status = hardware::EventFlag::createEventFlag(op_param.event_flag_word,
                                                    &ef_group);
      if (status != NO_ERROR) {  // check status
        LOG(ERROR) << "Creating event flag failure with code " << status;
        return false;
      }
      return queue_object->writeBlocking(
          data, op_param.data_size, op_param.read_notification,
          op_param.write_notification, op_param.time_out_nanos, ef_group);
    default:
      break;
  }

  LOG(ERROR) << "unknown operation";
  return false;
}

QueueId VtsFmqDriver::InsertQueue(unique_ptr<QueueInfo> queue_info) {
  map_mutex_.lock();
  size_t new_queue_id = fmq_map_.size();
  fmq_map_.emplace(new_queue_id, move(queue_info));
  map_mutex_.unlock();
  return new_queue_id;
}

bool VtsFmqDriver::ProcessUtilMethod(string type, bool sync, FmqOperation op,
                                     QueueId queue_id, size_t* result) {
  OperationParam params;
  params.op = op;
  params.queue_id = queue_id;
  RequestResult request_result;

  bool success = ProcessRequest(type, sync, params, &request_result);
  if (success) *result = request_result.sizet_val;  // store result in pointer
  return success;
}

}  // namespace vts
}  // namespace android
