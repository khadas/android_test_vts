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

#ifndef __VTS_SYSFUZZER_COMMON_SPECPARSER_SPECBUILDER_H__
#define __VTS_SYSFUZZER_COMMON_SPECPARSER_SPECBUILDER_H__

#include <queue>
#include <string>

#include "test/vts/proto/InterfaceSpecificationMessage.pb.h"

#include "fuzz_tester/FuzzerWrapper.h"


using namespace std;


#define DEFAULT_SPEC_DIR_PATH "/system/etc/"
#define SPEC_FILE_EXT ".vts"


namespace android {
namespace vts {

class FuzzerBase;
class InterfaceSpecification;

// Builder of an interface specification.
class SpecificationBuilder {
 public:
  // Constructor where the first argument is the path of a dir which contains
  // all available interface specification files.
  SpecificationBuilder(const string dir_path, int epoch_count,
                       const string& callback_socket_name);

  // scans the dir and returns an interface specification for a requested
  // component.
  vts::InterfaceSpecificationMessage* FindInterfaceSpecification(
      const int target_class, const int target_type, const float target_version,
      const string submodule_name = "");

  // Returns FuzzBase for a given interface specification, and adds all the
  // found functions to the fuzzing job queue.
  FuzzerBase* GetFuzzerBaseAndAddAllFunctionsToQueue(
      const vts::InterfaceSpecificationMessage& iface_spec_msg,
      const char* dll_file_name);

  const string& CallFunction(FunctionSpecificationMessage* func_msg);

  // Main function for the VTS system fuzzer where dll_file_name is the path of
  // a target component, spec_lib_file_path is the path of a specification
  // library file, and the rest three arguments are the basic information of
  // the target component.
  bool Process(
      const char* dll_file_name, const char* spec_lib_file_path,
      int target_class, int target_type, float target_version);

  bool LoadTargetComponent(
      const char* dll_file_name, const char* spec_lib_file_path,
      int target_class, int target_type, float target_version,
      const char* module_name);

  FuzzerBase* GetFuzzerBase(
      const vts::InterfaceSpecificationMessage& iface_spec_msg,
      const char* dll_file_name,
      const char* target_func_name);

  // Returns the loaded interface specification message.
  vts::InterfaceSpecificationMessage* GetInterfaceSpecification() const;

 private:
  // A FuzzerWrapper instance.
  FuzzerWrapper wrapper_;
  // the path of a dir which contains interface specification ASCII proto files.
  const string dir_path_;
  // the total number of epochs
  const int epoch_count_;
  // fuzzing job queue.
  queue<pair<vts::FunctionSpecificationMessage*, FuzzerBase*>> job_queue_;
  // Loaded interface specification message.
  vts::InterfaceSpecificationMessage* if_spec_msg_;
  // TODO: use unique_ptr
  char* spec_lib_file_path_;
  char* dll_file_name_;
  char* module_name_;
  // the server socket port # of the agent.
  const string& callback_socket_name_;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMMON_SPECPARSER_SPECBUILDER_H__
