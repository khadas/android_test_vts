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
#define LOG_TAG "VtsResourceManager"

#include "resource_manager/VtsResourceManager.h"

#include <android-base/logging.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/text_format.h>

#include "test/vts/proto/VtsResourceControllerMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

VtsResourceManager::VtsResourceManager() {}

VtsResourceManager::~VtsResourceManager() {}

void VtsResourceManager::ProcessFmqCommand(const FmqRequestMessage& fmq_request,
                                           FmqResponseMessage* fmq_response) {
  size_t sizet_result;
  int new_queue_id;
  bool success;

  LOG(DEBUG) << "Processing FMQ command in resource manager.";
  const string& data_type = fmq_request.data_type();
  bool sync = fmq_request.sync();
  size_t queue_size = fmq_request.queue_size();
  int queue_id = fmq_request.queue_id();
  bool blocking = fmq_request.blocking();
  bool reset_pointers = fmq_request.reset_pointers();

  switch (fmq_request.operation()) {
    case FMQ_PROTO_CREATE: {
      if (fmq_request.queue_id() == -1) {
        new_queue_id =
            fmq_driver_.CreateFmq(data_type, sync, queue_size, blocking);
      } else {
        new_queue_id =
            fmq_driver_.CreateFmq(data_type, sync, queue_id, reset_pointers);
      }
      fmq_response->set_queue_id(new_queue_id);
      break;
    }
    case FMQ_PROTO_READ:
    case FMQ_PROTO_READ_BLOCKING:
      ProcessFmqRead(fmq_request, fmq_response);
      break;
    case FMQ_PROTO_WRITE:
    case FMQ_PROTO_WRITE_BLOCKING: {
      ProcessFmqWrite(fmq_request, fmq_response);
      break;
    }
    case FMQ_PROTO_AVAILABLE_WRITE: {
      success = fmq_driver_.AvailableToWrite(data_type, sync, queue_id,
                                             &sizet_result);
      fmq_response->set_success(success);
      fmq_response->set_sizet_return_val(sizet_result);
      break;
    }
    case FMQ_PROTO_AVAILABLE_READ: {
      success =
          fmq_driver_.AvailableToRead(data_type, sync, queue_id, &sizet_result);
      fmq_response->set_success(success);
      fmq_response->set_sizet_return_val(sizet_result);
      break;
    }
    case FMQ_PROTO_GET_QUANTUM_SIZE: {
      success =
          fmq_driver_.GetQuantumSize(data_type, sync, queue_id, &sizet_result);
      fmq_response->set_success(success);
      fmq_response->set_sizet_return_val(sizet_result);
      break;
    }
    case FMQ_PROTO_GET_QUANTUM_COUNT: {
      success =
          fmq_driver_.GetQuantumCount(data_type, sync, queue_id, &sizet_result);
      fmq_response->set_success(success);
      fmq_response->set_sizet_return_val(sizet_result);
      break;
    }
    case FMQ_PROTO_IS_VALID: {
      success = fmq_driver_.IsValid(data_type, sync, queue_id);
      fmq_response->set_success(success);
      break;
    }
    default: {
      LOG(ERROR) << "unsupported operation.";
      break;
    }
  }
}

int VtsResourceManager::RegisterFmq(
    const VariableSpecificationMessage& queue_msg) {
  bool sync = queue_msg.type() == TYPE_FMQ_SYNC;
  // TODO: support user-defined types in the future, only support scalar types
  // for now.
  string data_type = queue_msg.fmq_value(0).scalar_type();

  size_t queue_desc_addr = queue_msg.fmq_value(0).fmq_desc_address();
  if (queue_desc_addr == 0) {
    LOG(ERROR) << "Invalid queue descriptor address."
               << "vtsc either didn't set the address or set a null pointer.";
    return -1;  // check for null pointer
  }
  return fmq_driver_.CreateFmq(data_type, sync, queue_desc_addr);
}

bool VtsResourceManager::GetQueueDescAddress(
    const VariableSpecificationMessage& queue_msg, size_t* result) {
  bool sync = queue_msg.type() == TYPE_FMQ_SYNC;
  // TODO: support user-defined types in the future, only support scalar types
  // for now.
  const string& data_type = queue_msg.fmq_value(0).scalar_type();
  int queue_id = queue_msg.fmq_value(0).fmq_id();
  bool success =
      fmq_driver_.GetQueueDescAddress(data_type, sync, queue_id, result);
  return success;
}

bool VtsResourceManager::ProcessFmqReadWriteInternal(
    FmqOp op, const string& data_type, bool sync, int queue_id, void* data,
    size_t data_size, int64_t time_out_nanos) {
  switch (op) {
    case FMQ_PROTO_WRITE: {
      return fmq_driver_.WriteFmq(data_type, sync, queue_id, data, data_size);
    }
    case FMQ_PROTO_WRITE_BLOCKING: {
      return fmq_driver_.WriteFmqBlocking(data_type, sync, queue_id, data,
                                          data_size, time_out_nanos);
    }
    case FMQ_PROTO_READ: {
      return fmq_driver_.ReadFmq(data_type, sync, queue_id, data, data_size);
    }
    case FMQ_PROTO_READ_BLOCKING: {
      return fmq_driver_.ReadFmqBlocking(data_type, sync, queue_id, data,
                                         data_size, time_out_nanos);
    }
    default: {
      LOG(ERROR) << "unsupported operation.";
      break;
    }
  }
  return false;
}

void VtsResourceManager::ProcessFmqWrite(const FmqRequestMessage& fmq_request,
                                         FmqResponseMessage* fmq_response) {
  void* data_buffer;
  bool success = false;
  FmqOp op = fmq_request.operation();
  const string& data_type = fmq_request.data_type();
  bool sync = fmq_request.sync();
  int queue_id = fmq_request.queue_id();
  size_t write_data_size = fmq_request.write_data_size();
  int64_t time_out_nanos = fmq_request.time_out_nanos();

  if (!data_type.compare("int8_t")) {
    vector<int8_t> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().int8_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("uint8_t")) {
    vector<uint8_t> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().uint8_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("int16_t")) {
    vector<int16_t> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().int16_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("uint16_t")) {
    vector<uint16_t> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().uint16_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("int32_t")) {
    vector<int32_t> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().int32_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("uint32_t")) {
    vector<uint32_t> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().uint32_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("int64_t")) {
    vector<int64_t> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().int64_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("uint64_t")) {
    vector<uint64_t> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().uint64_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("float_t")) {
    vector<float> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().float_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("double_t")) {
    vector<double> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().double_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  } else if (!data_type.compare("bool_t")) {
    vector<char> write_data(write_data_size);
    transform(fmq_request.write_data().cbegin(),
              fmq_request.write_data().cend(), write_data.begin(),
              [](const VariableSpecificationMessage& item) {
                return item.scalar_value().bool_t();
              });
    data_buffer = static_cast<void*>(&write_data[0]);
    success =
        ProcessFmqReadWriteInternal(op, data_type, sync, queue_id, data_buffer,
                                    write_data_size, time_out_nanos);
  }
  fmq_response->set_success(success);
}

// TODO: support user-defined types, only support primitive types now.
void VtsResourceManager::ProcessFmqRead(const FmqRequestMessage& fmq_request,
                                        FmqResponseMessage* fmq_response) {
  bool success = false;
  FmqOp op = fmq_request.operation();
  const string& data_type = fmq_request.data_type();
  bool sync = fmq_request.sync();
  int queue_id = fmq_request.queue_id();
  size_t read_data_size = fmq_request.read_data_size();
  int64_t time_out_nanos = fmq_request.time_out_nanos();

  if (!data_type.compare("int8_t")) {
    int8_t data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_int8_t(data_buffer[i]);
    }
  } else if (!data_type.compare("uint8_t")) {
    uint8_t data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_uint8_t(data_buffer[i]);
    }
  } else if (!data_type.compare("int16_t")) {
    int16_t data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_int16_t(data_buffer[i]);
    }
  } else if (!data_type.compare("uint16_t")) {
    uint16_t data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_uint16_t(data_buffer[i]);
    }
  } else if (!data_type.compare("int32_t")) {
    int32_t data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_int32_t(data_buffer[i]);
    }
  } else if (!data_type.compare("uint32_t")) {
    uint32_t data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_uint32_t(data_buffer[i]);
    }
  } else if (!data_type.compare("int64_t")) {
    int64_t data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_int64_t(data_buffer[i]);
    }
  } else if (!data_type.compare("uint64_t")) {
    uint64_t data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_uint64_t(data_buffer[i]);
    }
  } else if (!data_type.compare("float_t")) {
    float data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_float_t(data_buffer[i]);
    }
  } else if (!data_type.compare("double_t")) {
    double data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_double_t(data_buffer[i]);
    }
  } else if (!data_type.compare("bool_t")) {
    bool data_buffer[read_data_size];
    success = ProcessFmqReadWriteInternal(op, data_type, sync, queue_id,
                                          static_cast<void*>(data_buffer),
                                          read_data_size, time_out_nanos);
    fmq_response->clear_read_data();
    for (size_t i = 0; i < read_data_size; i++) {
      VariableSpecificationMessage* item = fmq_response->add_read_data();
      item->set_type(TYPE_SCALAR);
      item->set_scalar_type(data_type);
      (item->mutable_scalar_value())->set_bool_t(data_buffer[i]);
    }
  }
  fmq_response->set_success(success);
}

}  // namespace vts
}  // namespace android
