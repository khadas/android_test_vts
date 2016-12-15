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

#ifndef __VTS_SYSFUZZER_COMPILER_CODEGENBASE_H__
#define __VTS_SYSFUZZER_COMPILER_CODEGENBASE_H__

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "test/vts/sysfuzzer/common/proto/InterfaceSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

class CodeGenBase {
 public:
  explicit CodeGenBase(const char* input_vts_file_path, const char* vts_name);
  virtual ~CodeGenBase();

  // Generate both a C/C++ file and its header file.
  void GenerateAll(std::stringstream& cpp_ss,
                   std::stringstream& h_ss,
                   const InterfaceSpecificationMessage& message);

 protected:
  // Generates code for Fuzz(...) function body.
  virtual void GenerateCppBodyFuzzFunction(
      std::stringstream& cpp_ss,
      const InterfaceSpecificationMessage& message,
      const string& fuzzer_extended_class_name) = 0;

  // Generates header code to declare the C/C++ global functions.
  virtual void GenerateHeaderGlobalFunctionDeclarations(
      std::stringstream& h_ss,
      const string& function_prototype) = 0;

  // Generates code for the bodies of the C/C++ global functions.
  virtual void GenerateCppBodyGlobalFunctions(
      std::stringstream& cpp_ss,
      const string& function_prototype,
      const string& fuzzer_extended_class_name) = 0;

  // Generates code that opens the default namespaces.
  void GenerateOpenNameSpaces(std::stringstream& ss);

  // Generates code that closes the default namespaces.
  void GenerateCloseNameSpaces(std::stringstream& ss);

  // Generates code that starts the measurement.
  void GenerateCodeToStartMeasurement(std::stringstream& ss);

  // Generates code that stops the measurement.
  void GenerateCodeToStopMeasurement(std::stringstream& ss);

 private:
  const char* input_vts_file_path_;
  const char* vts_name_;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMPILER_CODEGENBASE_H__
