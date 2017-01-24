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

#ifndef SYSFUZZER_VTSCOMPILER_CODE_GEN_FUZZER_HALHIDLFUZZERCODEGENBASE_H_
#define SYSFUZZER_VTSCOMPILER_CODE_GEN_FUZZER_HALHIDLFUZZERCODEGENBASE_H_

#include "code_gen/fuzzer/FuzzerCodeGenBase.h"

using std::string;
using std::vector;

namespace android {
namespace vts {

// Generates fuzzer code for HIDL HALs.
class HalHidlFuzzerCodeGen : public FuzzerCodeGenBase {
 public:
  HalHidlFuzzerCodeGen(const ComponentSpecificationMessage &comp_spec,
                       const string &output_cpp_prefix)
      : FuzzerCodeGenBase(comp_spec, output_cpp_prefix) {}

  void GenerateBuildFile(Formatter &out) override;
  void GenerateSourceIncludeFiles(Formatter &out) override;
  void GenerateUsingDeclaration(Formatter &out) override;
  void GenerateReturnCallback(
      Formatter &out, const FunctionSpecificationMessage &func_spec) override;
  void GenerateLLVMFuzzerTestOneInput(
      Formatter &out, const FunctionSpecificationMessage &func_spec) override;
  string GetFuzzerBinaryName(
      const FunctionSpecificationMessage &func_spec) override;
  string GetFuzzerSourceName(
      const FunctionSpecificationMessage &func_spec) override;

 private:
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

#endif  // SYSFUZZER_VTSCOMPILER_CODE_GEN_FUZZER_HALHIDLFUZZERCODEGENBASE_H_
