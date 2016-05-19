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

#include "code_gen/HalCodeGen.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "test/vts/sysfuzzer/common/proto/InterfaceSpecificationMessage.pb.h"

#include "VtsCompilerUtils.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const HalCodeGen::kInstanceVariableName = "device_";


void HalCodeGen::GenerateCppBodyFuzzFunction(
    std::stringstream& cpp_ss,
    const InterfaceSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  cpp_ss << "bool " << fuzzer_extended_class_name << "::Fuzz(" << endl;
  cpp_ss << "    const FunctionSpecificationMessage& func_msg," << endl;
  cpp_ss << "    void** result) {" << endl;
  cpp_ss << "  const char* func_name = func_msg.name().c_str();" << endl;
  cpp_ss << "  cout << \"Function: \" << func_name << endl;" << endl;

  for (auto const& api : message.api()) {
    cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      cpp_ss << "    " << GetCppVariableType(arg) << " ";
      cpp_ss << "arg" << arg_count << " = ";
      if (arg_count == 0
          && arg.aggregate_type().size() == 1
          && !strncmp(arg.aggregate_type(0).c_str(),
                      message.original_data_structure_name().c_str(),
                      message.original_data_structure_name().length())) {
        cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">("
            << kInstanceVariableName << ")";
      } else {
        std::stringstream msg_ss;
        msg_ss << "func_msg.arg(" << arg_count << ")";
        string msg = msg_ss.str();

        cpp_ss << "(" << msg << ".primitive_value_size() > 0 ";
        cpp_ss << "|| " << msg << ".aggregate_value_size() > 0)? ";
        cpp_ss << GetCppInstanceType(arg, msg);
        cpp_ss << " : " << GetCppInstanceType(arg);
      }
      cpp_ss << ";" << endl;
      cpp_ss << "    cout << \"arg" << arg_count << " = \" << arg" << arg_count
          << " << endl;" << endl;
      arg_count++;
    }

    // actual function call
    GenerateCodeToStartMeasurement(cpp_ss);
    cpp_ss << "    ";
    if (api.return_type().primitive_type().size() == 1
        && !strcmp(api.return_type().primitive_type(0).c_str(), "void")) {
      cpp_ss << "*result = NULL;" << endl;
    } else {
      cpp_ss << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
    }
    cpp_ss << "reinterpret_cast<" << message.original_data_structure_name()
        << "*>(" << kInstanceVariableName << ")->" << api.name() << "(";
    if (arg_count > 0) cpp_ss << endl;

    for (int index = 0; index < arg_count; index++) {
      cpp_ss << "      arg" << index;
      if (index != (arg_count - 1)) {
        cpp_ss << "," << endl;
      }
    }
    if (api.return_type().primitive_type().size() == 0
        || (api.return_type().primitive_type().size() == 1
            && strcmp(api.return_type().primitive_type(0).c_str(), "void"))) {
      cpp_ss << "))";
    }
    cpp_ss << ");" << endl;
    GenerateCodeToStopMeasurement(cpp_ss);
    cpp_ss << "cout << \"called\" << endl;" << endl;
    cpp_ss << "    return true;" << endl;
    cpp_ss << "  }" << endl;
  }
  // TODO: if there were pointers, free them.
  cpp_ss << "  return false;" << endl;
  cpp_ss << "}" << endl;
}


void HalCodeGen::GenerateHeaderGlobalFunctionDeclarations(
    std::stringstream& h_ss,
    const string& function_prototype) {
  h_ss << "extern \"C\" {" << endl;
  h_ss << "extern " << function_prototype << ";" << endl;
  h_ss << "}" << endl;
}


void HalCodeGen::GenerateCppBodyGlobalFunctions(
    std::stringstream& cpp_ss,
    const string& function_prototype,
    const string& fuzzer_extended_class_name) {
  cpp_ss << "extern \"C\" {" << endl;
  cpp_ss << function_prototype << " {" << endl;
  cpp_ss << "  return (android::vts::FuzzerBase*) "
      << "new android::vts::" << fuzzer_extended_class_name << "();" << endl;
  cpp_ss << "}" << endl << endl;
  cpp_ss << "}" << endl;
}

}  // namespace vts
}  // namespace android
