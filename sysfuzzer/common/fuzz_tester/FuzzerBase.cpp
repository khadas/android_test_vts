/*
 * Copyright 2016 The Android Open Source Project
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

#include "fuzz_tester/FuzzerBase.h"

#include <string>
#include <iostream>

#include "test/vts/sysfuzzer/common/proto/InterfaceSpecificationMessage.pb.h"

#include "component_loader/DllLoader.h"
#include "utils/InterfaceSpecUtil.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

FuzzerBase::FuzzerBase(int target_class)
    : target_class_(target_class) {}


FuzzerBase::~FuzzerBase() {}


bool FuzzerBase::LoadTargetComponent(
    const char* target_dll_path, const char* module_name) {
  cout << __FUNCTION__ << ":" << __LINE__ << " " << "LoadTargetCompooent entry" << endl;
  target_dll_path_ = target_dll_path;
  if (!target_loader_.Load(target_dll_path_)) return false;
  cout << __FUNCTION__ << ":" << __LINE__ << " " << "LoadTargetCompooent loaded the target" << endl;
  if (target_class_ == LEGACY_HAL) return true;
  cout << __FUNCTION__ << ": loaded a non-legacy HAL file." << endl;
  device_ = target_loader_.GetHWDevice(module_name);
  cout << __FUNCTION__ << ": device_ " << device_ << endl;
  return (device_ != NULL);
}


bool FuzzerBase::Fuzz(const vts::InterfaceSpecificationMessage& message,
                      void** result) {
  cout << "Fuzzing target component: "
      << "class " << message.component_class()
      << " type " << message.component_type()
      << " version " << message.component_type_version() << endl;

  string function_name_prefix = GetFunctionNamePrefix(message);
  function_name_prefix_ = function_name_prefix.c_str();
  for (const vts::FunctionSpecificationMessage& func_msg : message.api()) {
    Fuzz(func_msg, result);
  }
  return true;
}

}  // namespace vts
}  // namespace android
