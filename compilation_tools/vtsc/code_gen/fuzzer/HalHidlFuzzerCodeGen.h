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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_FUZZER_HALHIDLFUZZERCODEGENBASE_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_FUZZER_HALHIDLFUZZERCODEGENBASE_H_

#include "code_gen/fuzzer/FuzzerCodeGenBase.h"

using std::string;
using std::vector;

namespace android {
namespace vts {

// Generates fuzzer code for HIDL HALs.
class HalHidlFuzzerCodeGen : public FuzzerCodeGenBase {
 public:
  HalHidlFuzzerCodeGen(const ComponentSpecificationMessage &comp_spec)
      : FuzzerCodeGenBase(comp_spec) {}

  void GenerateSourceIncludeFiles(Formatter &out) override;
  void GenerateUsingDeclaration(Formatter &out) override;
  void GenerateGlobalVars(Formatter &out) override;
  void GenerateLLVMFuzzerInitialize(Formatter &out) override;
  void GenerateLLVMFuzzerTestOneInput(Formatter &out) override;

 private:
  // Generates return callback function for HAL function being fuzzed.
  void GenerateReturnCallback(Formatter &out,
                              const FunctionSpecificationMessage &func_spec);
  // Generates function call.
  void GenerateHalFunctionCall(Formatter &out,
                               const FunctionSpecificationMessage &func_spec);
  // Returns name of pointer to hal instance.
  string GetHalPointerName();
  // Returns true iff callback can be omitted.
  bool CanElideCallback(const FunctionSpecificationMessage &func_spec);
  // Returns a vector of strings containing type names of function arguments.
  vector<string> GetFuncArgTypes(const FunctionSpecificationMessage &func_spec);
  // Name of return callback. Since we only fuzz one function, we'll need at
  // most one return callback.
  const string return_cb_name = "hidl_cb";
};

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_FUZZER_HALHIDLFUZZERCODEGENBASE_H_
