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

#include "specification_parser/SpecificationBuilder.h"

#include <dirent.h>

#include <iostream>
#include <queue>
#include <string>

#include "fuzz_tester/FuzzerBase.h"
#include "fuzz_tester/FuzzerWrapper.h"
#include "specification_parser/InterfaceSpecificationParser.h"

#include "test/vts/sysfuzzer/common/proto/InterfaceSpecificationMessage.pb.h"

namespace android {
namespace vts {

SpecificationBuilder::SpecificationBuilder(
    const string dir_path, int epoch_count)
    : dir_path_(dir_path),
      epoch_count_(epoch_count),
      if_spec_msg_(NULL) {}


vts::InterfaceSpecificationMessage*
SpecificationBuilder::FindInterfaceSpecification(
    const int target_class,
    const int target_type,
    const float target_version,
    const string submodule_name) {
  DIR* dir;
  struct dirent* ent;

  if (!(dir = opendir(dir_path_.c_str()))) {
    cerr << __FUNCTION__ << ": Can't opendir " << dir_path_ << endl;
    return NULL;
  }

  while ((ent = readdir(dir))) {
    if (string(ent->d_name).find(SPEC_FILE_EXT) != std::string::npos) {
      cout << __FUNCTION__ << ": Checking a file " << ent->d_name << endl;
      const string file_path = string(dir_path_) + "/" + string(ent->d_name);
      vts::InterfaceSpecificationMessage* message =
          new vts::InterfaceSpecificationMessage();
      if (InterfaceSpecificationParser::parse(file_path.c_str(), message)) {
        if (message->component_type() == target_type
            && message->component_type_version() == target_version) {
          if (submodule_name.length() > 0) {
            if (message->component_class() != HAL_SUBMODULE
                || message->original_data_structure_name() != submodule_name) {
              continue;
            }
          } else if (message->component_class() != target_class) continue;
          closedir(dir);
          return message;
        }
      }
      delete message;
    }
  }
  closedir(dir);
  return NULL;
}


FuzzerBase* SpecificationBuilder::GetFuzzerBaseAndAddAllFunctionsToQueue(
    const vts::InterfaceSpecificationMessage& iface_spec_msg,
    const char* dll_file_name) {
  FuzzerBase* fuzzer = wrapper_.GetFuzzer(iface_spec_msg);
  if (!fuzzer) {
    cerr << __FUNCTION__ << ": couldn't get a fuzzer base class" << endl;
    return NULL;
  }
  if (!fuzzer->LoadTargetComponent(dll_file_name)) return NULL;

  for (const vts::FunctionSpecificationMessage& func_msg : iface_spec_msg.api()) {
    cout << "Add a job " << func_msg.name() << endl;
    FunctionSpecificationMessage* func_msg_copy = func_msg.New();
    func_msg_copy->CopyFrom(func_msg);
    job_queue_.push(make_pair(func_msg_copy, fuzzer));
  }
  return fuzzer;
}


bool SpecificationBuilder::LoadTargetComponent(
    const char* dll_file_name,
    const char* spec_lib_file_path,
    int target_class,
    int target_type,
    float target_version) {
  if_spec_msg_ = FindInterfaceSpecification(
      target_class, target_type, target_version);
  if (!if_spec_msg_) {
    cerr << __FUNCTION__ <<
        ": no interface specification file found for "
        << "class " << target_class
        << " type " << target_type
        << " version " << target_version << endl;
    return false;
  }
  cout << "ifspec addr load " << if_spec_msg_ << endl;
  string output;
  if_spec_msg_->SerializeToString(&output);
  cout << "loaded text " << output.length() << endl;
  cout << "loaded text " << strlen(output.c_str()) << endl;
  cout << "loaded text " << output << endl;
  return true;
}


bool SpecificationBuilder::Process(
    const char* dll_file_name,
    const char* spec_lib_file_path,
    int target_class,
    int target_type,
    float target_version) {
  vts::InterfaceSpecificationMessage* interface_specification_message =
      FindInterfaceSpecification(target_class, target_type, target_version);
  cout << "ifspec addr " << interface_specification_message << endl;

  if (!interface_specification_message) {
    cerr << __FUNCTION__ <<
        ": no interface specification file found for "
        << "class " << target_class
        << " type " << target_type
        << " version " << target_version << endl;
    return false;
  }

  if (!wrapper_.LoadInterfaceSpecificationLibrary(spec_lib_file_path)) {
    return false;
  }

  if (!GetFuzzerBaseAndAddAllFunctionsToQueue(
          *interface_specification_message, dll_file_name)) return false;

  for (int i = 0; i < epoch_count_; i++) {
    // by default, breath-first-searching is used.
    if (job_queue_.empty()) {
      cout << "no more job to process; stopping after epoch " << i << endl;
      break;
    }

    pair<vts::FunctionSpecificationMessage*, FuzzerBase*> curr_job =
        job_queue_.front();
    job_queue_.pop();

    vts::FunctionSpecificationMessage* func_msg = curr_job.first;
    FuzzerBase* func_fuzzer = curr_job.second;

    void* result;
    cout << "Iteration " << (i + 1) << " Function " << func_msg->name() << endl;
    func_fuzzer->Fuzz(*func_msg, &result);
    if (func_msg->return_type().has_aggregate_type()) {
      if (result != NULL) {
        // loads that interface spec and enqueues all functions.
        cout << __FUNCTION__ << " return type: "
            << func_msg->return_type().aggregate_type() << endl;
        string submodule_name = func_msg->return_type().aggregate_type();
        while (!submodule_name.empty()
               && (std::isspace(submodule_name.back())
                   || submodule_name.back() == '*' )) {
          submodule_name.pop_back();
        }
        vts::InterfaceSpecificationMessage* iface_spec_msg =
            FindInterfaceSpecification(
                target_class, target_type, target_version, submodule_name);
        if (iface_spec_msg) {
          cout << __FUNCTION__ << " submodule found - " << submodule_name << endl;
          if (!GetFuzzerBaseAndAddAllFunctionsToQueue(
                  *iface_spec_msg, dll_file_name)) {
            return false;
          }
        } else {
          cout << __FUNCTION__ << " submodule not found - " << submodule_name << endl;
        }
      } else {
        cout << __FUNCTION__ << " return value = NULL" << endl;
      }
    }
  }

  return true;
}


vts::InterfaceSpecificationMessage*
SpecificationBuilder::GetInterfaceSpecification() const {
  cout << "ifspec addr get " << if_spec_msg_ << endl;
  return if_spec_msg_;
}

}  // namespace vts
}  // namespace android
