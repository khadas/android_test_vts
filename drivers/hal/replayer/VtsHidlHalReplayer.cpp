/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "VtsHidlHalReplayer.h"

#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "VtsProfilingUtil.h"
#include "driver_base/DriverBase.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

using namespace std;

static constexpr const char* kErrorString = "error";
static constexpr const char* kVoidString = "void";
static constexpr const int kInvalidDriverId = -1;

namespace android {
namespace vts {

void VtsHidlHalReplayer::ListTestServices(const string& trace_file) {
  // Parse the trace file to get the sequence of function calls.
  int fd = open(trace_file.c_str(), O_RDONLY);
  if (fd < 0) {
    cerr << "Can not open trace file: " << trace_file
         << " error: " << std::strerror(errno) << endl;
    return;
  }

  google::protobuf::io::FileInputStream input(fd);

  VtsProfilingRecord msg;
  set<string> registeredHalServices;
  while (readOneDelimited(&msg, &input)) {
    string package_name = msg.package();
    float version = msg.version();
    string interface_name = msg.interface();
    string service_fq_name =
        GetInterfaceFQName(package_name, version, interface_name);
    registeredHalServices.insert(service_fq_name);
  }
  for (string service : registeredHalServices) {
    cout << "hal_service: " << service << endl;
  }
}

bool VtsHidlHalReplayer::ReplayTrace(
    const string& trace_file, map<string, string>& hal_service_instances) {
  // Parse the trace file to get the sequence of function calls.
  int fd = open(trace_file.c_str(), O_RDONLY);
  if (fd < 0) {
    cerr << "Can not open trace file: " << trace_file
         << "error: " << std::strerror(errno);
    return false;
  }

  google::protobuf::io::FileInputStream input(fd);

  VtsProfilingRecord call_msg;
  VtsProfilingRecord expected_result_msg;
  while (readOneDelimited(&call_msg, &input) &&
         readOneDelimited(&expected_result_msg, &input)) {
    if (call_msg.event() != InstrumentationEventType::SERVER_API_ENTRY &&
        call_msg.event() != InstrumentationEventType::CLIENT_API_ENTRY &&
        call_msg.event() != InstrumentationEventType::SYNC_CALLBACK_ENTRY &&
        call_msg.event() != InstrumentationEventType::ASYNC_CALLBACK_ENTRY &&
        call_msg.event() != InstrumentationEventType::PASSTHROUGH_ENTRY) {
      cerr << "Expected a call message but got message with event: "
           << call_msg.event();
      continue;
    }
    if (expected_result_msg.event() !=
            InstrumentationEventType::SERVER_API_EXIT &&
        expected_result_msg.event() !=
            InstrumentationEventType::CLIENT_API_EXIT &&
        expected_result_msg.event() !=
            InstrumentationEventType::SYNC_CALLBACK_EXIT &&
        expected_result_msg.event() !=
            InstrumentationEventType::ASYNC_CALLBACK_EXIT &&
        expected_result_msg.event() !=
            InstrumentationEventType::PASSTHROUGH_EXIT) {
      cerr << "Expected a result message but got message with event: "
           << call_msg.event() << endl;
      continue;
    }

    string package_name = call_msg.package();
    float version = call_msg.version();
    string interface_name = call_msg.interface();
    string instance_name =
        GetInterfaceFQName(package_name, version, interface_name);
    string hal_service_name = "default";

    if (hal_service_instances.find(instance_name) ==
        hal_service_instances.end()) {
      cout << "Does not find service name for " << instance_name
           << ", using 'default' service name instead" << endl;
    } else {
      hal_service_name = hal_service_instances[instance_name];
    }

    cout << __func__ << ": replay function: " << call_msg.func_msg().name();

    int32_t driver_id = driver_manager_->GetDriverIdForHidlHalInterface(
        package_name, version, interface_name, hal_service_name);
    if (driver_id == kInvalidDriverId) {
      cerr << __func__ << ": couldn't get a driver base class" << endl;
      return false;
    }

    vts::FunctionCallMessage func_call_msg;
    func_call_msg.set_component_class(HAL_HIDL);
    func_call_msg.set_hal_driver_id(driver_id);
    *func_call_msg.mutable_api() = call_msg.func_msg();
    const string& result = driver_manager_->CallFunction(&func_call_msg);
    if (result == kVoidString || result == kErrorString) {
      cerr << __func__ << ": replay function fail." << endl;
      return false;
    }
    vts::FunctionSpecificationMessage result_msg;
    if (!google::protobuf::TextFormat::ParseFromString(result, &result_msg)) {
      cerr << __func__ << ": failed to parse result msg." << endl;
      return false;
    }
    if (!driver_manager_->VerifyResults(
            driver_id, expected_result_msg.func_msg(), result_msg)) {
      // Verification is not strict, i.e. if fail, output error message and
      // continue the process.
      cerr << __func__ << ": verification fail." << endl;
    }
    call_msg.Clear();
    expected_result_msg.Clear();
  }
  return true;
}

}  // namespace vts
}  // namespace android
