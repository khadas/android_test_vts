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

CodeGenBase::CodeGenBase(const char* input_vts_file_path,
                         const char* vts_name)
    : input_vts_file_path_(input_vts_file_path),
      vts_name_(vts_name) {}


CodeGenBase::~CodeGenBase() {}


void CodeGenBase::GenerateAll(std::stringstream& cpp_ss,
                              std::stringstream& h_ss,
                              const InterfaceSpecificationMessage& message) {
  cpp_ss << "#include \"" << string(input_vts_file_path_) << ".h\"" << endl;

  cpp_ss << "#include <iostream>" << endl;
  cpp_ss << "#include \"vts_datatype.h\"" << endl;
  for (auto const& header : message.header()) {
    cpp_ss << "#include " << header << endl;
  }
  GenerateOpenNameSpaces(cpp_ss);

  string component_name = message.original_data_structure_name();
  while (!component_name.empty()
         && (std::isspace(component_name.back())
             || component_name.back() == '*' )) {
    component_name.pop_back();
  }
  const auto pos = component_name.find_last_of(" ");
  if (pos != std::string::npos) {
    component_name = component_name.substr(pos + 1);
  }

  string fuzzer_extended_class_name;
  if (message.component_class() == HAL
      || message.component_class() == HAL_SUBMODULE) {
    fuzzer_extended_class_name = "FuzzerExtended_" + component_name;
  }

  h_ss << "#ifndef __VTS_SPEC_" << vts_name_ << "__" << endl;
  h_ss << "#define __VTS_SPEC_" << vts_name_ << "__" << endl;
  h_ss << endl;
  h_ss << "#define LOG_TAG \"" << fuzzer_extended_class_name << "\"" << endl;
  h_ss << "#include <utils/Log.h>" << endl;
  h_ss << "#include \"common/fuzz_tester/FuzzerBase.h\"" << endl;
  for (auto const& header : message.header()) {
    h_ss << "#include " << header << endl;
  }
  h_ss << "\n\n" << endl;
  GenerateOpenNameSpaces(h_ss);
  h_ss << "class " << fuzzer_extended_class_name << " : public FuzzerBase {"
      << endl;
  h_ss << " protected:" << endl;
  h_ss << "  bool Fuzz(const FunctionSpecificationMessage& func_msg," << endl;
  h_ss << "            void** result);" << endl;
  if (message.component_class() == HAL_SUBMODULE) {
    h_ss << "  void SetSubModule(" << component_name << "* submodule) {" << endl;
    h_ss << "    submodule_ = submodule;" << endl;
    h_ss << "  }" << endl;
    h_ss << endl;
    h_ss << " private:" << endl;
    h_ss << "  " << message.original_data_structure_name() << "* submodule_;"
        << endl;
  }
  h_ss << "};" << endl;

  string function_name_prefix = GetFunctionNamePrefix(message);

  cpp_ss << endl;
  GenerateCppBodyFuzzFunction(cpp_ss, message, fuzzer_extended_class_name);

  std::stringstream ss;
  // return type
  h_ss << endl;
  ss << "android::vts::FuzzerBase* " << endl;
  // function name
  ss << function_name_prefix << "(" << endl;
  ss << ")";

  GenerateHeaderGlobalFunctionDeclarations(h_ss, ss.str());

  GenerateCloseNameSpaces(cpp_ss);
  cpp_ss << endl << endl;

  GenerateCppBodyGlobalFunctions(cpp_ss, ss.str(), fuzzer_extended_class_name);

  GenerateCloseNameSpaces(h_ss);
  h_ss << "#endif" << endl;
}


void CodeGenBase::GenerateOpenNameSpaces(std::stringstream& ss) {
  ss << "namespace android {" << endl;
  ss << "namespace vts {" << endl;
}


void CodeGenBase::GenerateCloseNameSpaces(std::stringstream& ss) {
  ss << "}  // namespace vts" << endl;
  ss << "}  // namespace android" << endl;
}

}  // namespace vts
}  // namespace android
