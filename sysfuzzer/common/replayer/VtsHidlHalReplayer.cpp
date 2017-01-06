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
#include "replayer/VtsHidlHalReplayer.h"

#include <fstream>
#include <iostream>
#include <string>

#include <cutils/properties.h>
#include <google/protobuf/text_format.h>

#include "fuzz_tester/FuzzerBase.h"
#include "fuzz_tester/FuzzerWrapper.h"
#include "specification_parser/InterfaceSpecificationParser.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"
#include "test/vts/proto/VtsProfilingMessage.pb.h"
#include "utils/StringUtil.h"

namespace android {
namespace vts {

VtsHidlHalReplayer::VtsHidlHalReplayer(const char* spec_path,
                                       const char* callback_socket_name)
    : spec_path_(spec_path), callback_socket_name_(callback_socket_name) {}

bool VtsHidlHalReplayer::LoadComponentSpecification(
    const float version, const char* package, const char* component_name,
    ComponentSpecificationMessage* message) {
  if (!spec_path_ || !spec_path_[0]) {
    cerr << __func__ << "spec file not specified. " << endl;
    return false;
  }
  if (!message) {
    cerr << __func__ << "message could not be NULL. " << endl;
    return false;
  }
  cout << __func__ << ": Checking spec file " << spec_path_ << endl;
  if (InterfaceSpecificationParser::parse(spec_path_, message)) {
    if (message->component_class() != HAL_HIDL) {
      cerr << __func__ << ": only support Hidl Hal. " << endl;
      return false;
    }

    if (message->component_type_version() != version ||
        message->package() != package ||
        message->component_name() != component_name) {
      cerr << __func__
           << ": spec file mismatch. expect: target_version: " << version
           << ", package: " << package << ", component_name: " << component_name
           << ", actual: target_version: " << message->component_type_version()
           << ", package: " << message->package()
           << ", component_name: " << message->component_name();
      return false;
    }
  } else {
    cerr << __func__ << ": can not parse spec: " << spec_path_ << endl;
    return false;
  }
  return true;
}

bool VtsHidlHalReplayer::ParseTrace(const char* trace_file,
    vector<FunctionSpecificationMessage>* func_msgs,
    vector<FunctionSpecificationMessage>* result_msgs) {
  std::ifstream in(trace_file, std::ios::in);
  bool new_record = true;
  std::string record_str;
  std::string line;
  while (std::getline(in, line)) {
    // Assume records are separated by '\n'.
    if (line.empty()) {
      new_record = false;
    }
    if (new_record) {
      record_str += line + "\n";
    } else {
      unique_ptr<VtsProfilingRecord> record(new VtsProfilingRecord());
      if (!google::protobuf::TextFormat::MergeFromString(record_str,
                                                         record.get())) {
        cerr << __func__ << ": Can't parse a given function message: "
            << record_str << endl;
        return false;
      }
      if (record->event() == InstrumentationEventType::SERVER_API_ENTRY
          || record->event() == InstrumentationEventType::CLIENT_API_ENTRY
          || record->event() == InstrumentationEventType::SYNC_CALLBACK_ENTRY
          || record->event() == InstrumentationEventType::ASYNC_CALLBACK_ENTRY
          || record->event() == InstrumentationEventType::PASSTHROUGH_ENTRY) {
        func_msgs->push_back(record->func_msg());
      } else {
        result_msgs->push_back(record->func_msg());
      }
      new_record = true;
      record_str.clear();
    }
  }
  return true;
}

bool VtsHidlHalReplayer::ReplayTrace(const char* spec_lib_file_path,
                                     const char* trace_file,
                                     const float version, const char* package,
                                     const char* component_name) {
  ComponentSpecificationMessage interface_specification_message;
  if (!LoadComponentSpecification(version, package, component_name,
                                  &interface_specification_message)) {
    cerr << __func__ << ": can not load component spec: " << spec_path_
         << " for package: " << package << " version: " << version
         << " component_name: " << component_name << endl;
    return false;
  }

  if (!wrapper_.LoadInterfaceSpecificationLibrary(spec_lib_file_path)) {
    return false;
  }

  FuzzerBase* fuzzer = wrapper_.GetFuzzer(interface_specification_message);
  if (!fuzzer) {
    cerr << __func__ << ": couldn't get a fuzzer base class" << endl;
    return false;
  }

  // Get service for Hidl Hal.
  char get_sub_property[PROPERTY_VALUE_MAX];
  bool get_stub = false; /* default is binderized */
  if (property_get("vts.hidl.get_stub", get_sub_property, "") > 0) {
    if (!strcmp(get_sub_property, "true") ||
        !strcmp(get_sub_property, "True") || !strcmp(get_sub_property, "1")) {
      get_stub = true;
    }
  }
  const char* service_name = interface_specification_message.package().substr(
      interface_specification_message.package().find_last_of(".") + 1).c_str();
  if (!fuzzer->GetService(get_stub, service_name)) {
    cerr << __func__ << ": couldn't get service" << endl;
    return false;
  }

  // Parse the trace file to get the sequence of function calls.
  vector<FunctionSpecificationMessage> func_msgs;
  vector<FunctionSpecificationMessage> result_msgs;
  if (!ParseTrace(trace_file, &func_msgs, &result_msgs)) {
    cerr << __func__ << ": couldn't parse trace file: " << trace_file
         << endl;
    return false;
  }
  // Replay each function call from the trace and verify the results.
  for (size_t i = 0; i < func_msgs.size(); i++) {
    vts::FunctionSpecificationMessage func_msg = func_msgs[i];
    vts::FunctionSpecificationMessage expected_result_msg = result_msgs[i];
    cout << __func__ << ": replay function: " << func_msg.DebugString();
    vts::FunctionSpecificationMessage result_msg;
    if (!fuzzer->CallFunction(func_msg, callback_socket_name_, &result_msg)) {
      cerr << __func__ << ": replay function fail." << endl;
      return false;
    }
    if (!fuzzer->VerifyResults(expected_result_msg, result_msg)) {
      // Verification is not strict, i.e. if fail, output error message and
      // continue the process.
      cerr << __func__ << ": verification fail.\nexpected_result: "
           << expected_result_msg.DebugString() << "\nactual_result: "
           << result_msg.DebugString() << endl;
    }
  }
  return true;
}

}  // namespace vts
}  // namespace android
