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

#include <fcntl.h>
#include <sys/stat.h>

#include <android-base/logging.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/text_format.h>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"
#include "test/vts/proto/VtsResourceControllerMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

VtsResourceManager::VtsResourceManager() {}

VtsResourceManager::~VtsResourceManager() {}

void VtsResourceManager::ProcessHidlHandleCommand(
    const HidlHandleRequestMessage& hidl_handle_request,
    HidlHandleResponseMessage* hidl_handle_response) {
  HidlHandleOp operation = hidl_handle_request.operation();
  HandleId handle_id = hidl_handle_request.handle_id();
  HandleDataValueMessage handle_info = hidl_handle_request.handle_info();
  size_t read_data_size = hidl_handle_request.read_data_size();
  const void* write_data =
      static_cast<const void*>(hidl_handle_request.write_data().c_str());
  size_t write_data_size = hidl_handle_request.write_data().length();
  bool success = false;

  switch (operation) {
    case HANDLE_PROTO_CREATE_FILE: {
      if (handle_info.fd_val().size() == 0) {
        LOG(ERROR) << "No files to open.";
        break;
      }

      // TODO: currently only support opening a single file.
      // Support any file descriptor type in the future.
      FdMessage file_desc_info = handle_info.fd_val(0);
      if (file_desc_info.type() != FILE_TYPE) {
        LOG(ERROR) << "Currently only support file type.";
        break;
      }

      string filepath = file_desc_info.file_name();
      int flag;
      int mode = 0;
      // Translate the mode into C++ flags and modes.
      if (file_desc_info.file_mode_str() == "r" ||
          file_desc_info.file_mode_str() == "rb") {
        flag = O_RDONLY;
      } else if (file_desc_info.file_mode_str() == "w" ||
                 file_desc_info.file_mode_str() == "wb") {
        flag = O_WRONLY | O_CREAT | O_TRUNC;
        mode = S_IRWXU;  // User has the right to read/write/execute.
      } else if (file_desc_info.file_mode_str() == "a" ||
                 file_desc_info.file_mode_str() == "ab") {
        flag = O_WRONLY | O_CREAT | O_APPEND;
        mode = S_IRWXU;
      } else if (file_desc_info.file_mode_str() == "r+" ||
                 file_desc_info.file_mode_str() == "rb+" ||
                 file_desc_info.file_mode_str() == "r+b") {
        flag = O_RDWR;
      } else if (file_desc_info.file_mode_str() == "w+" ||
                 file_desc_info.file_mode_str() == "wb+" ||
                 file_desc_info.file_mode_str() == "w+b") {
        flag = O_RDWR | O_CREAT | O_TRUNC;
        mode = S_IRWXU;
      } else if (file_desc_info.file_mode_str() == "a+" ||
                 file_desc_info.file_mode_str() == "ab+" ||
                 file_desc_info.file_mode_str() == "a+b") {
        flag = O_RDWR | O_CREAT | O_APPEND;
        mode = S_IRWXU;
      } else if (file_desc_info.file_mode_str() == "x" ||
                 file_desc_info.file_mode_str() == "x+") {
        struct stat buffer;
        if (stat(filepath.c_str(), &buffer) == 0) {
          LOG(ERROR) << "Host side creates a file with mode x, "
                     << "but file already exists";
          break;
        }
        flag = O_CREAT;
        mode = S_IRWXU;
        if (file_desc_info.file_mode_str() == "x+") {
          flag |= O_RDWR;
        } else {
          flag |= O_WRONLY;
        }
      } else {
        LOG(ERROR) << "Unknown file mode.";
        break;
      }

      // Convert repeated int field into vector.
      vector<int> int_data(write_data_size);
      transform(handle_info.int_val().cbegin(), handle_info.int_val().cend(),
                int_data.begin(), [](const int& item) { return item; });
      // Call API on hidl_handle driver to create a file handle.
      int new_handle_id =
          hidl_handle_driver_.CreateFileHandle(filepath, flag, mode, int_data);
      hidl_handle_response->set_new_handle_id(new_handle_id);
      success = new_handle_id != -1;
      break;
    }
    case HANDLE_PROTO_READ_FILE: {
      char read_data[read_data_size];
      // Call API on hidl_handle driver to read the file.
      ssize_t read_success_bytes = hidl_handle_driver_.ReadFile(
          handle_id, static_cast<void*>(read_data), read_data_size);
      success = read_success_bytes != -1;
      hidl_handle_response->set_read_data(
          string(read_data, read_success_bytes));
      break;
    }
    case HANDLE_PROTO_WRITE_FILE: {
      // Call API on hidl_handle driver to write to the file.
      ssize_t write_success_bytes =
          hidl_handle_driver_.WriteFile(handle_id, write_data, write_data_size);
      success = write_success_bytes != -1;
      hidl_handle_response->set_write_data_size(write_success_bytes);
      break;
    }
    case HANDLE_PROTO_DELETE: {
      // Call API on hidl_handle driver to unregister the handle object.
      success = hidl_handle_driver_.UnregisterHidlHandle(handle_id);
      break;
    }
    default:
      LOG(ERROR) << "Unknown operation.";
      break;
  }
  hidl_handle_response->set_success(success);
}

void VtsResourceManager::ProcessHidlMemoryCommand(
    const HidlMemoryRequestMessage& hidl_memory_request,
    HidlMemoryResponseMessage* hidl_memory_response) {
  size_t mem_size = hidl_memory_request.mem_size();
  int mem_id = hidl_memory_request.mem_id();
  uint64_t start = hidl_memory_request.start();
  uint64_t length = hidl_memory_request.length();
  const string& write_data = hidl_memory_request.write_data();
  bool success = false;

  switch (hidl_memory_request.operation()) {
    case MEM_PROTO_ALLOCATE: {
      int new_mem_id = hidl_memory_driver_.Allocate(mem_size);
      hidl_memory_response->set_new_mem_id(new_mem_id);
      success = new_mem_id != -1;
      break;
    }
    case MEM_PROTO_START_READ: {
      success = hidl_memory_driver_.Read(mem_id);
      break;
    }
    case MEM_PROTO_START_READ_RANGE: {
      success = hidl_memory_driver_.ReadRange(mem_id, start, length);
      break;
    }
    case MEM_PROTO_START_UPDATE: {
      success = hidl_memory_driver_.Update(mem_id);
      break;
    }
    case MEM_PROTO_START_UPDATE_RANGE: {
      success = hidl_memory_driver_.UpdateRange(mem_id, start, length);
      break;
    }
    case MEM_PROTO_UPDATE_BYTES: {
      success = hidl_memory_driver_.UpdateBytes(mem_id, write_data.c_str(),
                                                length, start);
      break;
    }
    case MEM_PROTO_READ_BYTES: {
      char read_data[length];
      success = hidl_memory_driver_.ReadBytes(mem_id, read_data, length, start);
      hidl_memory_response->set_read_data(string(read_data, length));
      break;
    }
    case MEM_PROTO_COMMIT: {
      success = hidl_memory_driver_.Commit(mem_id);
      break;
    }
    case MEM_PROTO_GET_SIZE: {
      size_t result_mem_size;
      success = hidl_memory_driver_.GetSize(mem_id, &result_mem_size);
      hidl_memory_response->set_mem_size(result_mem_size);
      break;
    }
    default:
      LOG(ERROR) << "unknown operation in hidl_memory_driver.";
      break;
  }
  hidl_memory_response->set_success(success);
}

int VtsResourceManager::RegisterHidlMemory(
    const VariableSpecificationMessage& hidl_memory_msg) {
  size_t hidl_mem_address =
      hidl_memory_msg.hidl_memory_value().hidl_mem_address();
  if (hidl_mem_address == 0) {
    LOG(ERROR) << "Invalid queue descriptor address."
               << "vtsc either didn't set the address or set a null pointer.";
    return -1;  // check for null pointer
  }
  return hidl_memory_driver_.RegisterHidlMemory(hidl_mem_address);
}

bool VtsResourceManager::GetHidlMemoryAddress(
    const VariableSpecificationMessage& hidl_memory_msg, size_t* result) {
  int mem_id = hidl_memory_msg.hidl_memory_value().mem_id();
  bool success = hidl_memory_driver_.GetHidlMemoryAddress(mem_id, result);
  return success;
}

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
