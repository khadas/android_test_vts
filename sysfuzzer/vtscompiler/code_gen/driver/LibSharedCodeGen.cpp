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

#include "code_gen/driver/LibSharedCodeGen.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

#include "VtsCompilerUtils.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const LibSharedCodeGen::kInstanceVariableName = "sharedlib_";

void LibSharedCodeGen::GenerateCppBodyFuzzFunction(
    std::stringstream& cpp_ss, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  cpp_ss << "bool " << fuzzer_extended_class_name << "::Fuzz(" << endl;
  cpp_ss << "    FunctionSpecificationMessage* func_msg," << endl;
  cpp_ss << "    void** result, const string& callback_socket_name) {" << endl;
  cpp_ss << "  const char* func_name = func_msg->name().c_str();" << endl;
  cpp_ss << "  cout << \"Function: \" << func_name << endl;" << endl;

  for (auto const& api : message.interface().api()) {
    std::stringstream ss;

    cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg_count == 0 && arg.type() == TYPE_PREDEFINED &&
          !strncmp(arg.predefined_type().c_str(),
                   message.original_data_structure_name().c_str(),
                   message.original_data_structure_name().length()) &&
          message.original_data_structure_name().length() > 0) {
        cpp_ss << "    " << GetCppVariableType(arg) << " "
               << "arg" << arg_count << " = ";
        cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">("
               << kInstanceVariableName << ")";
      } else if (arg.type() == TYPE_SCALAR) {
        if (arg.scalar_type() == "char_pointer" ||
            arg.scalar_type() == "uchar_pointer") {
          if (arg.scalar_type() == "char_pointer") {
            cpp_ss << "    char ";
          } else {
            cpp_ss << "    unsigned char ";
          }
          cpp_ss << "arg" << arg_count
                 << "[func_msg->arg(" << arg_count
                 << ").string_value().length() + 1];" << endl;
          cpp_ss << "    if (func_msg->arg(" << arg_count
                 << ").type() == TYPE_SCALAR && "
                 << "func_msg->arg(" << arg_count
                 << ").string_value().has_message()) {" << endl;
          cpp_ss << "      strcpy(arg" << arg_count << ", "
                 << "func_msg->arg(" << arg_count << ").string_value()"
                 << ".message().c_str());" << endl;
          cpp_ss << "    } else {" << endl;
          cpp_ss << "   strcpy(arg" << arg_count << ", "
                 << GetCppInstanceType(arg) << ");" << endl;
          cpp_ss << "    }" << endl;
        } else {
          cpp_ss << "    " << GetCppVariableType(arg) << " "
                 << "arg" << arg_count << " = ";
          cpp_ss << "(func_msg->arg(" << arg_count
                 << ").type() == TYPE_SCALAR && "
                 << "func_msg->arg(" << arg_count
                 << ").scalar_value().has_" << arg.scalar_type() << "()) ? ";
          if (arg.scalar_type() == "void_pointer") {
            cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">(";
          }
          cpp_ss << "func_msg->arg(" << arg_count << ").scalar_value()."
                 << arg.scalar_type() << "()";
          if (arg.scalar_type() == "void_pointer") {
            cpp_ss << ")";
          }
          cpp_ss << " : " << GetCppInstanceType(arg);
        }
      } else {
        cpp_ss << "    " << GetCppVariableType(arg) << " "
               << "arg" << arg_count << " = ";
        cpp_ss << GetCppInstanceType(arg);
      }
      cpp_ss << ";" << endl;
      cpp_ss << "    cout << \"arg" << arg_count << " = \" << arg" << arg_count
             << " << endl;" << endl;
      arg_count++;
    }

    cpp_ss << "    ";
    cpp_ss << "typedef void* (*";
    cpp_ss << "func_type_" << api.name() << ")(...";
    cpp_ss << ");" << endl;

    // actual function call
    if (!api.has_return_type() || api.return_type().type() == TYPE_VOID) {
      cpp_ss << "*result = NULL;" << endl;
    } else {
      cpp_ss << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
    }
    cpp_ss << "    ";
    cpp_ss << "((func_type_" << api.name() << ") "
           << "target_loader_.GetLoaderFunction(\"" << api.name() << "\"))(";
    // cpp_ss << "reinterpret_cast<" << message.original_data_structure_name()
    //    << "*>(" << kInstanceVariableName << ")->" << api.name() << "(";

    if (arg_count > 0) cpp_ss << endl;

    for (int index = 0; index < arg_count; index++) {
      cpp_ss << "      arg" << index;
      if (index != (arg_count - 1)) {
        cpp_ss << "," << endl;
      }
    }
    if (api.has_return_type() || api.return_type().type() != TYPE_VOID) {
      cpp_ss << "))";
    }
    cpp_ss << ");" << endl;
    cpp_ss << "    return true;" << endl;
    cpp_ss << "  }" << endl;
  }
  // TODO: if there were pointers, free them.
  cpp_ss << "  return false;" << endl;
  cpp_ss << "}" << endl;
}

void LibSharedCodeGen::GenerateCppBodyGetAttributeFunction(
    std::stringstream& cpp_ss,
    const ComponentSpecificationMessage& /*message*/,
    const string& fuzzer_extended_class_name) {
  cpp_ss << "bool " << fuzzer_extended_class_name << "::GetAttribute(" << endl;
  cpp_ss << "    FunctionSpecificationMessage* func_msg," << endl;
  cpp_ss << "    void** result) {" << endl;
  cpp_ss << "  const char* func_name = func_msg->name().c_str();" << endl;
  cpp_ss
      << "  cout << \"Function: \" << __func__ << \" '\" << func_name << \"'\" << endl;"
      << endl;

  cpp_ss << "  cerr << \"attribute not supported for shared lib yet\" << endl;"
         << endl;
  cpp_ss << "  return false;" << endl;
  cpp_ss << "}" << endl;
}

void LibSharedCodeGen::GenerateHeaderGlobalFunctionDeclarations(
    std::stringstream& /*h_ss*/, const string& /*function_prototype*/) {}

}  // namespace vts
}  // namespace android
