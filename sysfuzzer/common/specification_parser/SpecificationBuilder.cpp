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
      epoch_count_(epoch_count) {}


vts::InterfaceSpecificationMessage*
SpecificationBuilder::FindInterfaceSpecification(
    const int target_class,
    const int target_type,
    const float target_version) {
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
        if (message->component_class() == target_class
            && message->component_type() == target_type
            && message->component_type_version() == target_version) {
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


bool SpecificationBuilder::Process(
    const char* dll_file_name,
    const char* spec_lib_file_path,
    int target_class,
    int target_type,
    float target_version) {
  vts::InterfaceSpecificationMessage* interface_specification_message =
      FindInterfaceSpecification(target_class, target_type, target_version);
  if (!interface_specification_message) {
    cerr << __FUNCTION__ <<
        ": no interface specification file found for "
        << "class " << target_class
        << " type " << target_type
        << " version " << target_version << endl;
    return false;
  }
  FuzzerWrapper wrapper = FuzzerWrapper();

  if (!wrapper.LoadInterfaceSpecificationLibrary(spec_lib_file_path)) {
    return false;
  }
  FuzzerBase* fuzzer = wrapper.GetFuzzer(*interface_specification_message);
  if (!fuzzer) {
    cerr << __FUNCTION__ << ": coult't get a fuzzer base class" << endl;
    return false;
  }
  if (!fuzzer->LoadTargetComponent(dll_file_name)) return -1;
  for (int i = 0; i < epoch_count_; i++) {
    fuzzer->Fuzz(*interface_specification_message);
  }
  return true;
}


}  // namespace vts
}  // namespace android
