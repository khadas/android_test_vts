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

#include "code_gen/CodeGenBase.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "utils/InterfaceSpecUtil.h"

#include "VtsCompilerUtils.h"

using namespace std;

namespace android {
namespace vts {

CodeGenBase::CodeGenBase(const char* input_vts_file_path, const char* vts_name)
    : input_vts_file_path_(input_vts_file_path), vts_name_(vts_name) {}

CodeGenBase::~CodeGenBase() {}

void CodeGenBase::GenerateAll(std::stringstream& cpp_ss,
                              std::stringstream& h_ss,
                              const InterfaceSpecificationMessage& message) {
  cpp_ss << "#include \"" << string(input_vts_file_path_) << ".h\"" << endl;

  cpp_ss << "#include <iostream>" << endl;
  cpp_ss << "#include \"vts_datatype.h\"" << endl;
  cpp_ss << "#include \"vts_measurement.h\"" << endl;
  for (auto const& header : message.header()) {
    cpp_ss << "#include " << header << endl;
  }
  if (message.component_class() == HAL_HIDL && message.has_component_name()) {
    cpp_ss << "#include <" << message.component_name() << ".h>" << endl;
    cpp_ss << "#include <Bp" << message.component_name().substr(1) << ".h>" << endl;
    for (const auto& import : message.import()) {
      string mutable_import = import;
      ReplaceSubString(mutable_import, ".", "/");
      cpp_ss << "#include <" << mutable_import << ".h>" << endl;
    }
  }
  GenerateOpenNameSpaces(cpp_ss, message);

  string component_name = GetComponentName(message);
  if (component_name.empty()) {
    cerr << __func__ << ":" << __LINE__ << " error component_name is empty"
         << endl;
    exit(-1);
  }
  string fuzzer_extended_class_name;
  if (message.component_class() == HAL_CONVENTIONAL ||
      message.component_class() == HAL_CONVENTIONAL_SUBMODULE ||
      message.component_class() == HAL_HIDL ||
      message.component_class() == HAL_LEGACY ||
      message.component_class() == LIB_SHARED) {
    fuzzer_extended_class_name = "FuzzerExtended_" + component_name;
  }

  GenerateAllHeader(fuzzer_extended_class_name, h_ss, message);
  cpp_ss << endl << endl;
  if (message.component_class() == HAL_CONVENTIONAL ||
      message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
    GenerateCppBodyCallbackFunction(cpp_ss, message,
                                    fuzzer_extended_class_name);
  }

  cpp_ss << endl;
  GenerateCppBodyFuzzFunction(cpp_ss, message, fuzzer_extended_class_name);

  std::stringstream ss;
  // return type
  ss << "android::vts::FuzzerBase* " << endl;
  // function name
  string function_name_prefix = GetFunctionNamePrefix(message);
  ss << function_name_prefix << "(" << endl;
  ss << ")";

  GenerateCppBodyGlobalFunctions(cpp_ss, ss.str(), fuzzer_extended_class_name);

  GenerateCloseNameSpaces(cpp_ss);
}

void CodeGenBase::GenerateAllHeader(
    const string& fuzzer_extended_class_name, std::stringstream& h_ss,
    const InterfaceSpecificationMessage& message) {
  h_ss << "#ifndef __VTS_SPEC_" << vts_name_ << "__" << endl;
  h_ss << "#define __VTS_SPEC_" << vts_name_ << "__" << endl;
  h_ss << endl;
  h_ss << "#include <stdio.h>" << endl;
  h_ss << "#include <stdarg.h>" << endl;
  h_ss << "#include <stdlib.h>" << endl;
  h_ss << "#define LOG_TAG \"" << fuzzer_extended_class_name << "\"" << endl;
  h_ss << "#include <utils/Log.h>" << endl;
  h_ss << "#include \"common/fuzz_tester/FuzzerBase.h\"" << endl;
  h_ss << "#include \"common/fuzz_tester/FuzzerCallbackBase.h\"" << endl;
  for (auto const& header : message.header()) {
    h_ss << "#include " << header << endl;
  }
  if (message.component_class() == HAL_HIDL && message.has_component_name()) {
    h_ss << "#include <" << message.component_name() << ".h>" << endl;
    h_ss << "#include <Bp" << message.component_name().substr(1) << ".h>" << endl;
  }
  h_ss << "\n\n" << endl;
  GenerateOpenNameSpaces(h_ss, message);

  GenerateClassHeader(fuzzer_extended_class_name, h_ss, message);

  string function_name_prefix = GetFunctionNamePrefix(message);

  std::stringstream ss;
  // return type
  h_ss << endl;
  ss << "android::vts::FuzzerBase* " << endl;
  // function name
  ss << function_name_prefix << "(" << endl;
  ss << ")";

  GenerateHeaderGlobalFunctionDeclarations(h_ss, ss.str());

  GenerateCloseNameSpaces(h_ss);
  h_ss << "#endif" << endl;
}

void CodeGenBase::GenerateClassHeader(
    const string& fuzzer_extended_class_name, std::stringstream& h_ss,
    const InterfaceSpecificationMessage& message) {
  h_ss << "class " << fuzzer_extended_class_name << " : public FuzzerBase {"
       << endl;
  h_ss << " public:" << endl;
  h_ss << "  " << fuzzer_extended_class_name << "() : FuzzerBase(";

  if (message.component_class() == HAL_CONVENTIONAL) {
    h_ss << "HAL_CONVENTIONAL)";
  } else if (message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
    h_ss << "HAL_CONVENTIONAL_SUBMODULE)";
  } else if (message.component_class() == HAL_HIDL) {
    h_ss << "HAL_HIDL), hw_binder_proxy_()";
  } else if (message.component_class() == HAL_LEGACY) {
    h_ss << "HAL_LEGACY)";
  } else if (message.component_class() == LIB_SHARED) {
    h_ss << "LIB_SHARED)";
  }

  h_ss << " { }" << endl;
  h_ss << " protected:" << endl;
  h_ss << "  bool Fuzz(FunctionSpecificationMessage* func_msg," << endl;
  h_ss << "            void** result, const string& callback_socket_name);"
       << endl;

  // produce Fuzz method(s) for sub_struct(s).
  for (auto const& sub_struct : message.sub_struct()) {
    GenerateFuzzFunctionForSubStruct(h_ss, sub_struct, "_");
  }

  if (message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
    string component_name = GetComponentName(message);
    h_ss << "  void SetSubModule(" << component_name << "* submodule) {"
         << endl;
    h_ss << "    submodule_ = submodule;" << endl;
    h_ss << "  }" << endl;
    h_ss << endl;
    h_ss << " private:" << endl;
    h_ss << "  " << message.original_data_structure_name() << "* submodule_;"
         << endl;
  }
  if (message.component_class() == HAL_HIDL) {
    h_ss << " private:" << endl;
    h_ss << "  Bp" << message.component_name().substr(1) << " hw_binder_proxy_;" << endl;
  }
  h_ss << "};" << endl;
}

void CodeGenBase::GenerateFuzzFunctionForSubStruct(
    std::stringstream& h_ss, const StructSpecificationMessage& message,
    const string& parent_path) {
  h_ss << "  bool Fuzz_" << parent_path << message.name()
       << "(FunctionSpecificationMessage* func_msg," << endl;
  h_ss << "            void** result, const string& callback_socket_name);"
       << endl;

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateFuzzFunctionForSubStruct(h_ss, sub_struct,
                                     parent_path + message.name() + "_");
  }
}

void CodeGenBase::GenerateOpenNameSpaces(
    std::stringstream& ss, const InterfaceSpecificationMessage& message) {
  if (message.component_class() == HAL_HIDL && message.has_package()) {
    ss << "using namespace ";
    string name = message.package();
    ReplaceSubString(name, ".", "::");
    ss << name << ";" << endl;
  }

  ss << "namespace android {" << endl;
  ss << "namespace vts {" << endl;
}

void CodeGenBase::GenerateCloseNameSpaces(std::stringstream& ss) {
  ss << "}  // namespace vts" << endl;
  ss << "}  // namespace android" << endl;
}

void CodeGenBase::GenerateCodeToStartMeasurement(std::stringstream& ss) {
  ss << "    VtsMeasurement vts_measurement;" << endl;
  ss << "    vts_measurement.Start();" << endl;
}

void CodeGenBase::GenerateCodeToStopMeasurement(std::stringstream& ss) {
  ss << "    vector<float>* measured = vts_measurement.Stop();" << endl;
  ss << "    cout << \"time \" << (*measured)[0] << endl;" << endl;
}

string CodeGenBase::GetComponentName(
    const InterfaceSpecificationMessage& message) {
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

void CodeGenBase::GenerateCppBodyCallbackFunction(
    std::stringstream& /*cpp_ss*/,
    const InterfaceSpecificationMessage& /*message*/,
    const string& /*fuzzer_extended_class_name*/) {}

}  // namespace vts
}  // namespace android
