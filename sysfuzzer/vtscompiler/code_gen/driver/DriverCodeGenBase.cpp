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

#include "code_gen/driver/DriverCodeGenBase.h"

#include <hidl-util/Formatter.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "utils/InterfaceSpecUtil.h"

#include "VtsCompilerUtils.h"
#include "utils/StringUtil.h"

using namespace std;

namespace android {
namespace vts {

void DriverCodeGenBase::GenerateAll(
    Formatter& header_out, Formatter& source_out,
    const ComponentSpecificationMessage& message) {
  string component_name = GetComponentName(message);
  if (component_name.empty()) {
    cerr << __func__ << ":" << __LINE__ << " error component_name is empty"
         << "\n";
    exit(-1);
  }
  string fuzzer_extended_class_name = "FuzzerExtended_" + component_name;

  GenerateHeaderFile(header_out, message, fuzzer_extended_class_name);
  GenerateSourceFile(source_out, message, fuzzer_extended_class_name);
}


void DriverCodeGenBase::GenerateHeaderFile(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  out << "#ifndef __VTS_SPEC_" << vts_name_ << "__" << "\n";
  out << "#define __VTS_SPEC_" << vts_name_ << "__" << "\n";
  out << "\n";

  out << "#define LOG_TAG \"" << fuzzer_extended_class_name << "\"" << "\n";

  GenerateHeaderIncludeFiles(out, message, fuzzer_extended_class_name);

  GenerateOpenNameSpaces(out, message);
  GenerateClassHeader(out, message, fuzzer_extended_class_name);
  out << "\n\n";
  GenerateHeaderGlobalFunctionDeclarations(out, message);
  GenerateCloseNameSpaces(out);
  out << "#endif" << "\n";
}

void DriverCodeGenBase::GenerateSourceFile(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  GenerateSourceIncludeFiles(out, message, fuzzer_extended_class_name);
  out << "\n\n";
  GenerateOpenNameSpaces(out, message);
  GenerateClassImpl(out, message, fuzzer_extended_class_name);
  GenerateCppBodyGlobalFunctions(out, message, fuzzer_extended_class_name);
  GenerateCloseNameSpaces(out);
}

void DriverCodeGenBase::GenerateClassHeader(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  out << "class " << fuzzer_extended_class_name << " : public FuzzerBase {"
      << "\n";
  out << " public:" << "\n";

  out.indent();
  GenerateClassConstructionFunction(out, message, fuzzer_extended_class_name);
  out.unindent();

  out << " protected:" << "\n";

  out.indent();
  out << "bool Fuzz(FunctionSpecificationMessage* func_msg, void** result, "
      << "const string& callback_socket_name);\n";
  out << "bool CallFunction(FunctionSpecificationMessage* func_msg, "
      << "void** result, const string& callback_socket_name);\n";
  out << "bool VerifyResults(FunctionSpecificationMessage* func_msg, "
      << "vector<void *> results);\n";
  out << "bool GetAttribute(FunctionSpecificationMessage* func_msg, "
      << "void** result);\n";

  // Produce Fuzz method(s) for sub_struct(s).
  for (auto const& sub_struct : message.interface().sub_struct()) {
    GenerateFuzzFunctionForSubStruct(out, sub_struct, "_");
  }
  // Generate additional function declarations if any.
  GenerateAdditionalFuctionDeclarations(out, message,
                                        fuzzer_extended_class_name);
  out.unindent();

  out << " private:" << "\n";

  out.indent();
  // Generate declarations of private members if any.
  GeneratePrivateMemberDeclarations(out, message);
  out.unindent();

  out << "};\n";
}

void DriverCodeGenBase::GenerateClassImpl(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  GenerateCppBodyCallbackFunction(out, message, fuzzer_extended_class_name);
  GenerateCppBodyFuzzFunction(out, message, fuzzer_extended_class_name);
  GenerateCppBodyGetAttributeFunction(out, message, fuzzer_extended_class_name);
  GenerateDriverFunctionImpl(out, message, fuzzer_extended_class_name);
  GenerateVerificationFunctionImpl(out, message, fuzzer_extended_class_name);
}

void DriverCodeGenBase::GenerateHeaderIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message, const string&) {
  for (auto const& header : message.header()) {
    out << "#include " << header << "\n";
  }
  out << "\n\n";
  out << "#include <stdio.h>" << "\n";
  out << "#include <stdarg.h>" << "\n";
  out << "#include <stdlib.h>" << "\n";
  out << "#include <string.h>" << "\n";
  out << "#include <utils/Log.h>" << "\n";

  out << "#include <fuzz_tester/FuzzerBase.h>" << "\n";
  out << "#include <fuzz_tester/FuzzerCallbackBase.h>" << "\n";
  out << "\n\n";
}

void DriverCodeGenBase::GenerateSourceIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message, const string&) {
  out << "#include \"" << input_vts_file_path_ << ".h\"" << "\n";

  for (auto const& header : message.header()) {
    out << "#include " << header << "\n";
  }
  out << "#include \"vts_datatype.h\"" << "\n";
  out << "#include \"vts_measurement.h\"" << "\n";
  out << "#include <iostream>" << "\n";
}

void DriverCodeGenBase::GenerateHeaderGlobalFunctionDeclarations(Formatter& out,
    const ComponentSpecificationMessage& message) {
  string function_name_prefix = GetFunctionNamePrefix(message);

  out << "extern \"C\" {" << "\n";
  out << "extern " << "android::vts::FuzzerBase* " << function_name_prefix
      << "();\n";
  out << "}" << "\n";
}

void DriverCodeGenBase::GenerateCppBodyGlobalFunctions(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  string function_name_prefix = GetFunctionNamePrefix(message);

  out << "extern \"C\" {" << "\n";
  out << "android::vts::FuzzerBase* " << function_name_prefix << "() {\n";
  out.indent();
  out << "return (android::vts::FuzzerBase*) " << "new android::vts::"
      << fuzzer_extended_class_name << "();\n";
  out.unindent();
  out << "}\n\n";
  out << "}\n";
}

void DriverCodeGenBase::GenerateFuzzFunctionForSubStruct(
    Formatter& out, const StructSpecificationMessage& message,
    const string& parent_path) {
  out.indent();
  out << "bool Fuzz_" << parent_path << message.name()
      << "(FunctionSpecificationMessage* func_msg," << "\n";
  out << "            void** result, const string& callback_socket_name);"
      << "\n";

  out << "bool GetAttribute_" << parent_path << message.name()
      << "(FunctionSpecificationMessage* func_msg," << "\n";
  out << "            void** result);"
      << "\n";

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateFuzzFunctionForSubStruct(out, sub_struct,
                                     parent_path + message.name() + "_");
  }
  out.unindent();
}

void DriverCodeGenBase::GenerateDriverFunctionImpl(Formatter& out,
    const ComponentSpecificationMessage& /*message*/,
    const string& fuzzer_extended_class_name) {
  out << "bool " << fuzzer_extended_class_name
      << "::CallFunction(FunctionSpecificationMessage*, "
      << "void**, const string&) {\n";
  out.indent();
  out << "/* No implementation yet. */\n";
  out << "return true;\n";
  out.unindent();
  out << "}\n";
}

void DriverCodeGenBase::GenerateVerificationFunctionImpl(Formatter& out,
    const ComponentSpecificationMessage& /*message*/,
    const string& fuzzer_extended_class_name) {
  out << "bool " << fuzzer_extended_class_name
      << "::VerifyResults(FunctionSpecificationMessage*, vector<void *>) {\n";
  out.indent();
  out << "/* No implementation yet. */\n";
  out << "return true;\n";
  out.unindent();
  out << "}\n";
}

void DriverCodeGenBase::GenerateNamespaceName(
    Formatter& out, const ComponentSpecificationMessage& message) {
  if (message.component_class() == HAL_HIDL && message.has_package()) {
    string name = message.package();
    ReplaceSubString(name, ".", "::");
    out << name << "::"
        << GetVersionString(message.component_type_version(), true);
  } else {
    cerr << __func__ << ":" << __LINE__ << " no namespace" << "\n";
    exit(-1);
  }
}

void DriverCodeGenBase::GenerateOpenNameSpaces(
    Formatter& out, const ComponentSpecificationMessage& message) {
  if (message.component_class() == HAL_HIDL && message.has_package()) {
    out << "using namespace ";
    GenerateNamespaceName(out, message);
    out << ";" << "\n";
  }

  out << "namespace android {" << "\n";
  out << "namespace vts {" << "\n";
}

void DriverCodeGenBase::GenerateCloseNameSpaces(Formatter& out) {
  out << "}  // namespace vts" << "\n";
  out << "}  // namespace android" << "\n";
}

void DriverCodeGenBase::GenerateCodeToStartMeasurement(Formatter& out) {
  out << "VtsMeasurement vts_measurement;" << "\n";
  out << "vts_measurement.Start();" << "\n";
}

void DriverCodeGenBase::GenerateCodeToStopMeasurement(Formatter& out) {
  out << "vector<float>* measured = vts_measurement.Stop();" << "\n";
  out << "cout << \"time \" << (*measured)[0] << endl;" << "\n";
}

string DriverCodeGenBase::GetComponentName(
    const ComponentSpecificationMessage& message) {
  if (!message.component_name().empty()) {
    return message.component_name();
  }

  string component_name = message.original_data_structure_name();
  while (!component_name.empty() && (std::isspace(component_name.back()) ||
                                     component_name.back() == '*')) {
    component_name.pop_back();
  }
  const auto pos = component_name.find_last_of(" ");
  if (pos != std::string::npos) {
    component_name = component_name.substr(pos + 1);
  }
  return component_name;
}
}  // namespace vts
}  // namespace android
