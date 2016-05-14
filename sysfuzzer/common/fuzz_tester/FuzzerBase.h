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

#ifndef __VTS_SYSFUZZER_COMMON_FUZZER_BASE_H__
#define __VTS_SYSFUZZER_COMMON_FUZZER_BASE_H__

#include "component_loader/DllLoader.h"

#include "test/vts/sysfuzzer/common/proto/InterfaceSpecificationMessage.pb.h"


using namespace std;

namespace android {
namespace vts {

class FuzzerBase {
 public:
  FuzzerBase(int target_class);
  virtual ~FuzzerBase();

  // Loads a target component where the argument is the file path.
  // Returns true iff successful.
  bool LoadTargetComponent(const char* target_dll_path);

  // Fuzz tests the loaded component using the provided interface specification.
  // Returns true iff the testing is conducted completely.
  bool Fuzz(const vts::InterfaceSpecificationMessage& message, void** result);

  // Actual implementation of routines to test a specific function using the
  // provided function interface specification message.
  // Returns true iff the testing is conducted completely.
  virtual bool Fuzz(const vts::FunctionSpecificationMessage& func_msg,
                    void** result) {
    return false;
  };

 protected:
  // a pointer to a HAL data structure of the loaded component.
  struct hw_device_t* device_;

  // DLL Loader class.
  DllLoader target_loader_;

 private:
  // a pointer to the string which contains the loaded component.
  const char* target_dll_path_;

  // function name prefix.
  const char* function_name_prefix_;

  // target class
  const int target_class_;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMMON_FUZZER_BASE_H__
