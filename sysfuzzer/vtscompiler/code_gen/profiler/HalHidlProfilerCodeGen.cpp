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

#include "HalHidlProfilerCodeGen.h"
#include "utils/InterfaceSpecUtil.h"
#include "VtsCompilerUtils.h"

namespace android {
namespace vts {

void HalHidlProfilerCodeGen::GenerateProfilerForScalarVariable(Formatter& out,
  const VariableSpecificationMessage& val, const std::string& arg_name,
  const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_SCALAR);\n";
  out << arg_name << "->mutable_scalar_value()->set_" << val.scalar_type()
      << "(" << arg_value << ");\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForStringVariable(Formatter& out,
  const VariableSpecificationMessage&, const std::string& arg_name,
  const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_STRING);\n";
  out << arg_name << "->mutable_string_value()->set_message" << "(" << arg_value
      << ".c_str());\n";
  out << arg_name << "->mutable_string_value()->set_length" << "(" << arg_value
      << ".size());\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForEnumVariable(Formatter& out,
  const VariableSpecificationMessage&, const std::string& arg_name,
  const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_ENUM);\n";
  out << arg_name << "->mutable_enum_value()->add_scalar_value().set_uint8_t(static_cast<uint8_t>"
      << "(" << arg_value << "));\n";
  out << arg_name << "->mutable_enum_value()->set_scalar_type(\"uint8_t\");\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForVectorVariable(Formatter& out,
  const VariableSpecificationMessage& val, const std::string& arg_name,
  const std::string& arg_value) {
  out << "for (int i = 0; i < (int)" << arg_value << ".size(); i++) {\n";
  out.indent();
  std::string vector_element_name = arg_name + "_vector_i";
  out << "auto *" << vector_element_name << " = " << arg_name
      << "->add_vector_value();\n";
  GenerateProfilerForTypedVariable(out, val.vector_value(0),
                                   vector_element_name, arg_value + "[i]");
  out.unindent();
  out << "}\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForArrayVariable(Formatter& out,
  const VariableSpecificationMessage& val, const std::string& arg_name,
  const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_ARRAY);\n";
  out << "for (int i = 0; i < " << val.vector_size() << "; i++) {\n";
  out.indent();
  std::string array_element_name = arg_name + "_array_i";
  out << "auto *" << array_element_name << " = " << arg_name
      << "->add_vector_value();\n";
  GenerateProfilerForTypedVariable(out, val.vector_value(0), array_element_name,
                                   arg_value + "[i]");
  out.unindent();
  out << "}\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForStructVariable(Formatter& out,
  const VariableSpecificationMessage& val, const std::string& arg_name,
  const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_STRUCT);\n";
  // For predefined type, call the corresponding profile method.
  if (val.struct_value().size() == 0 && val.has_predefined_type()) {
    out << "profile__" << val.predefined_type() << "(" << arg_name << ", "
        << arg_value << ");\n";
  } else {
    for (const auto struct_field : val.struct_value()) {
      // skip sub_type.
      // TODO(zhuoyao): update the condition once vts distinguish between
      // sub_types and fields.
      // TODO(zhuoyao): handle union sub_type once vts support union type.
      if (struct_field.type() == TYPE_STRUCT
          && !struct_field.has_predefined_type()) {
        continue;
      }
      std::string struct_field_name = arg_name + "_" + struct_field.name();
      out << "auto *" << struct_field_name << " = " << arg_name
          << "->add_struct_value();\n";
      GenerateProfilerForTypedVariable(out, struct_field, struct_field_name,
                                       arg_value + "." + struct_field.name());
    }
  }
}

void HalHidlProfilerCodeGen::GenerateProfilerForUnionVariable(Formatter& out,
  const VariableSpecificationMessage&, const std::string& arg_name,
  const std::string&) {
  out << arg_name << "->set_type(TYPE_UNION);\n";
  // TODO(zhuoyao): add profiling method once vts support union type.
}

void HalHidlProfilerCodeGen::GenerateProfilerForHidlCallbackVariable(
  Formatter& out, const VariableSpecificationMessage&,
  const std::string& arg_name, const std::string&) {
  out << arg_name << "->set_type(TYPE_HIDL_CALLBACK);\n";
  // TODO(zhuoyao): figure the right way to profile hidl callback type.
}

void HalHidlProfilerCodeGen::GenerateProfilerForMethod(Formatter& out,
  const FunctionSpecificationMessage& method) {
  out << "FunctionSpecificationMessage msg;\n";
  out << "switch (event) {\n";
  out.indent();
  out << "case SERVER_API_ENTRY:\n";
  out << "{\n";
  out.indent();
  ComponentSpecificationMessage message;
  for (int i = 0; i < method.arg().size(); i++) {
    const VariableSpecificationMessage arg = method.arg(i);
    std::string arg_name = "arg_" + std::to_string(i);
    std::string arg_value = "arg_val_" + std::to_string(i);
    out << "auto *" << arg_name << " = msg.add_arg();\n";
    // TODO(zhuoyao): GetCppVariableType does not support array type for now.
    out << GetCppVariableType(arg, &message) << " *" << arg_value
        << " = reinterpret_cast<" << GetCppVariableType(arg, &message)
        << "*> ((*args)[" << i << "]);\n";
    GenerateProfilerForTypedVariable(out, arg, arg_name,
                                     "(*" + arg_value + ")");
  }
  out << "break;\n";
  out.unindent();
  out << "}\n";
  out << "case SERVER_API_EXIT:\n";
  out << "{\n";
  out.indent();
  for (int i = 0; i < method.return_type_hidl().size(); i++) {
    const VariableSpecificationMessage arg = method.return_type_hidl(i);
    std::string result_name = "result_" + std::to_string(i);
    std::string result_value = "result_val_" + std::to_string(i);
    out << "auto *" << result_name << " = msg.add_return_type_hidl();\n";
    out << GetCppVariableType(arg, &message) << " *" << result_value
        << " = reinterpret_cast<" << GetCppVariableType(arg, &message)
        << "*> ((*args)[" << i << "]);\n";

    GenerateProfilerForTypedVariable(out, arg, result_name,
                                     "(*" + result_value + ")");
  }
  out << "break;\n";
  out.unindent();
  out << "}\n";
  out << "default:\n";
  out << "{\n";
  out.indent();
  out << "LOG(WARNING) << \"not supported. \";\n";
  out << "break;\n";
  out.unindent();
  out << "}\n";
  out.unindent();
  out << "}\n";
  out << "VtsProfilingInterface::getInstance().AddTraceEvent(msg);\n";
  out.unindent();
  out << "}\n";
}

void HalHidlProfilerCodeGen::GenerateHeaderIncludeFiles(Formatter& out,
  const ComponentSpecificationMessage& message) {
  // Basic includes.
  out << "#include <android-base/logging.h>\n";
  out << "#include <hidl/HidlSupport.h>\n";
  out << "#include <test/vts/proto/ComponentSpecificationMessage.pb.h>\n";
  out << "#include <VtsProfilingInterface.h>\n";
  out << "\n";

  std::string package_path = GetPackage(message);
  ReplaceSubString(package_path, ".", "/");

  // Include generated hal classes.
  out << "#include <" << package_path << "/" << GetPackageVersion(message)
      << "/" << GetComponentName(message) << ".h>\n";

  // Include imported classes.
  for (const auto& import : message.import()) {
    string mutable_import = import;
    string base_filename = mutable_import.substr(
        mutable_import.find_last_of("::") + 1);

    out << "#include <" << package_path << "/" << GetPackageVersion(message)
        << "/" << base_filename << ".h>\n";
  }
  out << "\n\n";
}

void HalHidlProfilerCodeGen::GenerateSourceIncludeFiles(Formatter& out,
  const ComponentSpecificationMessage&) {
  // First include the corresponding profiler header file.
  out << "#include \"" << input_vts_file_path_ << ".h\"\n";

  // Include the header file for types profiler.
  std::string header_path_prefix = input_vts_file_path_.substr(
      0, input_vts_file_path_.find_last_of("\\/"));
  out << "#include \"" << header_path_prefix << "/types.vts.h\"\n";
  out << "\n";
}

void HalHidlProfilerCodeGen::GenerateUsingDeclaration(Formatter& out,
  const ComponentSpecificationMessage& message) {
  std::string package_path = GetPackage(message);
  ReplaceSubString(package_path, ".", "::");

  out << "using namespace android::hardware;\n";
  out << "using namespace ";
  out << package_path << "::"
      << GetVersionString(message.component_type_version(), true) << ";\n";
  out << "\n";
}

void HalHidlProfilerCodeGen::GenerateProfierSanityCheck(Formatter& out,
  const ComponentSpecificationMessage& message) {
  out << "if (strcmp(package, \"" << GetPackage(message) << "\") != 0) {\n";
  out.indent();
  out << "LOG(WARNING) << \"incorrect package.\";\n";
  out << "return;\n";
  out.unindent();
  out << "}\n";

  out << "if (strcmp(version, \"" << GetPackageVersion(message)
      << "\") != 0) {\n";
  out.indent();
  out << "LOG(WARNING) << \"incorrect version.\";\n";
  out << "return;\n";
  out.unindent();
  out << "}\n";

  out << "if (strcmp(interface, \"" << GetComponentName(message)
      << "\") != 0) {\n";
  out.indent();
  out << "LOG(WARNING) << \"incorrect interface.\";\n";
  out << "return;\n";
  out.unindent();
  out << "}\n";
}

}  // namespace vts
}  // namespace android
