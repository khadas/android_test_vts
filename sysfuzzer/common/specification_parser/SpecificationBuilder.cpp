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

#include "test/vts/runners/host/proto/InterfaceSpecificationMessage.pb.h"
#include <google/protobuf/text_format.h>

namespace android {
namespace vts {

SpecificationBuilder::SpecificationBuilder(
    const string dir_path, int epoch_count, int agent_port)
    : dir_path_(dir_path),
      epoch_count_(epoch_count),
      if_spec_msg_(NULL),
      module_name_(NULL),
      agent_port_(agent_port) {}


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
            if (message->component_class() != HAL_CONVENTIONAL_SUBMODULE
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


FuzzerBase* SpecificationBuilder::GetFuzzerBase(
    const vts::InterfaceSpecificationMessage& iface_spec_msg,
    const char* dll_file_name,
    const char* target_func_name) {
  cout << __func__ << ":" << __LINE__ << " " << "entry" << endl;
  FuzzerBase* fuzzer = wrapper_.GetFuzzer(iface_spec_msg);
  if (!fuzzer) {
    cerr << __FUNCTION__ << ": couldn't get a fuzzer base class" << endl;
    return NULL;
  }

  // TODO: don't load multiple times. reuse FuzzerBase*.
  cout << __func__ << ":" << __LINE__ << " " << "got fuzzer" << endl;
  if (!fuzzer->LoadTargetComponent(dll_file_name)) {
    cerr << __FUNCTION__ << ": couldn't load target component file, "
        << dll_file_name << endl;
    return NULL;
  }
  cout << __func__ << ":" << __LINE__ << " " << "loaded target comp" << endl;

  return fuzzer;
  /*
   * TODO: now always return the fuzzer. this change is due to the difficulty
   * in checking nested apis although that's possible. need to check whether
   * Fuzz() found the function, while still distinguishing the difference
   * between that and defined but non-set api.
  if (!strcmp(target_func_name, "#Open")) return fuzzer;

  for (const vts::FunctionSpecificationMessage& func_msg : iface_spec_msg.api()) {
    cout << "checking " << func_msg.name() << endl;
    if (!strcmp(target_func_name, func_msg.name().c_str())) {
      return fuzzer;
    }
  }
  return NULL;
  */
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
    float target_version,
    const char* module_name) {
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
  spec_lib_file_path_ = (char*) malloc(strlen(spec_lib_file_path) + 1);
  strcpy(spec_lib_file_path_, spec_lib_file_path);

  dll_file_name_ = (char*) malloc(strlen(dll_file_name) + 1);
  strcpy(dll_file_name_, dll_file_name);

  // cout << "ifspec addr load at " << if_spec_msg_ << endl;
  string output;
  if_spec_msg_->SerializeToString(&output);
  cout << "loaded ifspec length " << output.length() << endl;
  // cout << "loaded text " << strlen(output.c_str()) << endl;
  // cout << "loaded text " << output << endl;

  module_name_ = (char*) malloc(strlen(module_name) + 1);
  strcpy(module_name_, module_name);
  cout << __FUNCTION__ << ":" << __LINE__ << " module_name " << module_name_
      << endl;
  return true;
}


const string empty_string = string();

const string& SpecificationBuilder::CallFunction(
    FunctionSpecificationMessage* func_msg) {
  if (!wrapper_.LoadInterfaceSpecificationLibrary(spec_lib_file_path_)) {
    return empty_string;
  }
  cout << __func__ << " " << "loaded if_spec lib" << endl;
  cout << __func__ << " " << dll_file_name_ << " " << func_msg->name() << endl;

  FuzzerBase* func_fuzzer = GetFuzzerBase(
      *if_spec_msg_, dll_file_name_, func_msg->name().c_str());
  cout << __func__ << ":" << __LINE__ << endl;
  if (!func_fuzzer) {
    cerr << "can't find FuzzerBase for " << func_msg->name() << " using "
        << dll_file_name_ << endl;
    return empty_string;
  }

  if (func_msg->name() == "#Open") {
    cout << __func__ << ":" << __LINE__ << endl;
    if (func_msg->arg().size() > 0) {
      cout << __func__ << " "
          << func_msg->arg(0).string_value().message()
          << endl;
      func_fuzzer->OpenConventionalHal(
          func_msg->arg(0).string_value().message().c_str());
    } else {
      cout << __func__ << " no arg" << endl;
      func_fuzzer->OpenConventionalHal();
    }
    cout << __func__ << " opened" << endl;
    // return the return value from open;
    if (func_msg->return_type().has_type()) {
      cout << __func__ << " return_type exists" << endl;
      // TODO handle when the size > 1.
      if (!strcmp(func_msg->return_type().scalar_type().c_str(), "int32_t")) {
        cout << __func__ << " return_type is int32_t" << endl;
        func_msg->mutable_return_type()->mutable_scalar_value()->set_int32_t(0);
        cout << "result " << endl;
        // todo handle more types;
        string* output = new string();
        google::protobuf::TextFormat::PrintToString(*func_msg, output);
        return *output;
      }
    }
    cerr << __func__ << " return_type unknown" << endl;
    string* output = new string();
    google::protobuf::TextFormat::PrintToString(*func_msg, output);
    return *output;
  }
  cout << __func__ << ":" << __LINE__ << endl;

  void* result;
  func_fuzzer->FunctionCallBegin();
  cout << __func__ << " Call Function " << func_msg->name() << " parent_path("
      << func_msg->parent_path() << ")" << endl;
  if (!func_fuzzer->Fuzz(func_msg, &result, agent_port_)) {
    cerr << __func__ << " function not found - todo handle more explicitly" << endl;
    return *(new string("error"));
  }
  cout << __func__ << ": called" << endl;

  // set coverage data.
  vector<unsigned>* coverage = func_fuzzer->FunctionCallEnd();
  if (coverage && coverage->size() > 0) {
    for (unsigned int index = 0; index < coverage->size(); index++) {
      func_msg->mutable_coverage_data()->Add(coverage->at(index));
    }
  }

  if (func_msg->return_type().type() == TYPE_PREDEFINED) {
    // TODO: actually handle this case.
    if (result != NULL) {
      // loads that interface spec and enqueues all functions.
      cout << __func__ << " return type: "
          << func_msg->return_type().type() << endl;
    } else {
      cout << __func__ << " return value = NULL" << endl;
    }
    cerr << __func__ << " todo: support aggregate" << endl;
    string* output = new string();
    google::protobuf::TextFormat::PrintToString(*func_msg, output);
    return *output;
  } else if (func_msg->return_type().type() == TYPE_SCALAR) {
    // TODO handle when the size > 1.
    if (!strcmp(func_msg->return_type().scalar_type().c_str(), "int32_t")) {
      func_msg->mutable_return_type()->mutable_scalar_value()->set_int32_t(
          *((int*)(&result)));
      cout << "result " << endl;
      // todo handle more types;
      string* output = new string();
      google::protobuf::TextFormat::PrintToString(*func_msg, output);
      return *output;
    }
  }
  return *(new string("void"));
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
    func_fuzzer->Fuzz(func_msg, &result, agent_port_);
    if (func_msg->return_type().type() == TYPE_PREDEFINED) {
      if (result != NULL) {
        // loads that interface spec and enqueues all functions.
        cout << __FUNCTION__ << " return type: "
            << func_msg->return_type().predefined_type() << endl;
        // TODO: handle the case when size > 1
        string submodule_name = func_msg->return_type().predefined_type();
        while (!submodule_name.empty()
               && (std::isspace(submodule_name.back())
                   || submodule_name.back() == '*')) {
          submodule_name.pop_back();
        }
        vts::InterfaceSpecificationMessage* iface_spec_msg =
            FindInterfaceSpecification(
                target_class, target_type, target_version, submodule_name);
        if (iface_spec_msg) {
          cout << __FUNCTION__ << " submodule found - " << submodule_name
              << endl;
          if (!GetFuzzerBaseAndAddAllFunctionsToQueue(
                  *iface_spec_msg, dll_file_name)) {
            return false;
          }
        } else {
          cout << __FUNCTION__ << " submodule not found - "
              << submodule_name << endl;
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
