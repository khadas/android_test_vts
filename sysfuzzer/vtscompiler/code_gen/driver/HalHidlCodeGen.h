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

#ifndef __VTS_SYSFUZZER_COMPILER_HALHIDLCODEGEN_H__
#define __VTS_SYSFUZZER_COMPILER_HALHIDLCODEGEN_H__

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

#include "code_gen/driver/DriverCodeGenBase.h"

using namespace std;

namespace android {
namespace vts {

class HalHidlCodeGen : public DriverCodeGenBase {
 public:
  HalHidlCodeGen(const char* input_vts_file_path, const string& vts_name)
      : DriverCodeGenBase(input_vts_file_path, vts_name) {}

 protected:
  void GenerateClassHeader(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateClassImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateCppBodyFuzzFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  virtual void GenerateDriverFunctionImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateVerificationFunctionImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateCppBodyGetAttributeFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateCppBodyCallbackFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateClassConstructionFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateHeaderGlobalFunctionDeclarations(Formatter& out,
      const ComponentSpecificationMessage& message) override;

  void GenerateCppBodyGlobalFunctions(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateHeaderIncludeFiles(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateSourceIncludeFiles(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateAdditionalFuctionDeclarations(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GeneratePrivateMemberDeclarations(Formatter& out,
      const ComponentSpecificationMessage& message) override;

  void GenerateCppBodyFuzzFunction(Formatter& out,
      const StructSpecificationMessage& message,
      const string& fuzzer_extended_class_name,
      const string& original_data_structure_name, const string& parent_path);

  void GenerateCppBodySyncCallbackFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  void GenerateSubStructFuzzFunctionCall(Formatter& out,
      const StructSpecificationMessage& message, const string& parent_path);

  // Generates a scalar type in C/C++.
  void GenerateScalarTypeInC(Formatter& out, const string& type);

  // Generates the driver function implementation for attributes defined within
  // an interface or in a types.hal.
  void GenerateDriverImplForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the driver code for a typed variable.
  void GenerateDriverImplForTypedVariable(Formatter& out,
      const VariableSpecificationMessage& val, const string& arg_name,
      const string& arg_value_name);

  // Generates the verification function declarations for attributes defined
  // within an interface or in a types.hal.
  void GenerateVerificationDeclForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the verification function declarations for attributes defined
  // within an interface or in a types.hal.
  void GenerateVerificationImplForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the verification code for a typed variable.
  void GenerateVerificationForTypedVariable(Formatter& out,
      const VariableSpecificationMessage& val, const string& result_value,
      const string& expected_result);

  // Generates the random function implementation for attributes defined within
  // an interface or in a types.hal.
  void GenerateRandomFunctionForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the getService function implementation for an interface.
  void GenerateGetServiceImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  // instance variable name (e.g., device_);
  static const char* const kInstanceVariableName;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMPILER_HALHIDLCODEGEN_H__
