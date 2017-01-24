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

#include "HalHidlFuzzerCodeGen.h"
#include "VtsCompilerUtils.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

using std::cerr;
using std::cout;
using std::endl;
using std::vector;

namespace android {
namespace vts {

void HalHidlFuzzerCodeGen::GenerateBuildFile(Formatter &out) {
  GenerateWarningComment(out);
  for (const auto &func_spec : comp_spec_.interface().api()) {
    if (func_spec.arg_size() == 0) {
      continue;
    }
    out << "cc_binary {\n";
    out.indent();
    out << "name: \"" << GetFuzzerBinaryName(func_spec) << "\",\n";
    out << "srcs: [\"" << GetFuzzerSourceName(func_spec) << ".cpp\"],\n";
    string comp_version = GetVersionString(comp_spec_.component_type_version());
    string hal_lib = comp_spec_.package() + "@" + comp_version;
    out << "include_dirs: [\"external/llvm/lib/Fuzzer\"],\n";
    out << "shared_libs: [\n";
    out.indent();
    out << "\"" << hal_lib << "\",\n";
    out << "\"libutils\",\n";
    out << "\"libhidlbase\",\n";
    out << "\"libhidltransport\",\n";
    out << "\"libhardware\",\n";
    out.unindent();
    out << "],\n";
    out << "static_libs: [\"libLLVMFuzzer\"],\n";
    out << "cflags: [\n";
    out.indent();
    out << "\"-Wno-unused-parameter\",\n";
    out << "\"-fno-omit-frame-pointer\",\n";
    out.unindent();
    out << "],\n";
    out.unindent();
    out << "}\n\n";
  }
}

void HalHidlFuzzerCodeGen::GenerateSourceIncludeFiles(Formatter &out) {
  out << "#include <FuzzerInterface.h>\n";

  string package_path = comp_spec_.package();
  ReplaceSubString(package_path, ".", "/");
  string comp_version = GetVersionString(comp_spec_.component_type_version());
  string comp_name = comp_spec_.component_name();

  out << "#include <" << package_path << "/" << comp_version << "/" << comp_name
      << ".h>\n";
  out << "\n";
}

void HalHidlFuzzerCodeGen::GenerateUsingDeclaration(Formatter &out) {
  string package_path = comp_spec_.package();
  ReplaceSubString(package_path, ".", "::");
  string comp_version =
      GetVersionString(comp_spec_.component_type_version(), true);

  out << "using namespace ::" << package_path << "::" << comp_version << ";\n";
  out << "using namespace ::android::hardware;\n";
  out << "\n";
}

void HalHidlFuzzerCodeGen::GenerateReturnCallback(
    Formatter &out, const FunctionSpecificationMessage &func_spec) {
  if (CanElideCallback(func_spec)) {
    return;
  }
  out << "// No-op. Only need this to make HAL function call.\n";
  out << "auto " << return_cb_name << " = [](";
  size_t num_cb_arg = func_spec.return_type_hidl_size();
  for (size_t i = 0; i < num_cb_arg; ++i) {
    out << "auto arg" << i << ((i != num_cb_arg - 1) ? ", " : "");
  }
  out << "){};\n\n";
}
void HalHidlFuzzerCodeGen::GenerateLLVMFuzzerTestOneInput(
    Formatter &out, const FunctionSpecificationMessage &func_spec) {
  string prefix = "android.hardware.";
  string hal_name = comp_spec_.package().substr(prefix.size());

  out << "extern \"C\" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t "
         "size) {\n";
  out.indent();
  out << "static ::android::sp<" << comp_spec_.component_name() << "> "
      << hal_name << " = " << comp_spec_.component_name()
      << "::getService(true);\n";
  out << "if (" << hal_name << "== nullptr) { return 0; }\n\n";

  vector<string> types{GetFuncArgTypes(func_spec)};
  for (size_t i = 0; i < types.size(); ++i) {
    out << "size_t type_size" << i << " = sizeof(" << types[i] << ");\n";
    out << "if (size < type_size" << i << ") { return 0; }\n";
    out << "size -= type_size" << i << ";\n";
    out << types[i] << " arg" << i << ";\n";
    out << "memcpy(&arg" << i << ", data, type_size" << i << ");\n";
    out << "data += type_size" << i << ";\n\n";
  }

  out << hal_name << "->" << func_spec.name() << "(";
  for (size_t i = 0; i < types.size(); ++i) {
    out << "arg" << i << ((i != types.size() - 1) ? ", " : "");
  }
  if (!CanElideCallback(func_spec)) {
    if (func_spec.arg_size() > 0) {
      out << ", ";
    }
    out << return_cb_name;
  }
  out << ");\n";
  out << "return 0;\n";
  out.unindent();
  out << "}\n\n";
}

string HalHidlFuzzerCodeGen::GetFuzzerBinaryName(
    const FunctionSpecificationMessage &func_spec) {
  string package = comp_spec_.package();
  string version = GetVersionString(comp_spec_.component_type_version());
  string func_name = func_spec.name();
  return package + "@" + version + "-vts.func_fuzzer-" + func_name;
}

string HalHidlFuzzerCodeGen::GetFuzzerSourceName(
    const FunctionSpecificationMessage &func_spec) {
  string comp_name = comp_spec_.component_name();
  string func_name = func_spec.name();
  return comp_name + "_" + func_name + "_fuzzer";
}

bool HalHidlFuzzerCodeGen::CanElideCallback(
    const FunctionSpecificationMessage &func_spec) {
  // Can't elide callback for void or tuple-returning methods.
  if (func_spec.return_type_hidl_size() != 1) {
    return false;
  }
  if (func_spec.return_type_hidl(0).type() == TYPE_SCALAR ||
      func_spec.return_type_hidl(0).type() == TYPE_ENUM) {
    return true;
  }
  return false;
}

vector<string> HalHidlFuzzerCodeGen::GetFuncArgTypes(
    const FunctionSpecificationMessage &func_spec) {
  vector<string> types{};
  for (const auto &var_spec : func_spec.arg()) {
    string type = GetCppVariableType(var_spec);
    types.emplace_back(GetCppVariableType(var_spec));
  }
  return types;
}

}  // namespace vts
}  // namespace android
