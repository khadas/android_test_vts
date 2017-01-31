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

#include "code_gen/driver/HalHidlCodeGen.h"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

#include "VtsCompilerUtils.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const HalHidlCodeGen::kInstanceVariableName = "hw_binder_proxy_";

void HalHidlCodeGen::GenerateCppBodyCallbackFunction(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& /*fuzzer_extended_class_name*/) {
  if (endsWith(message.component_name(), "Callback")) {
    out << "\n";
    for (const auto& api : message.interface().api()) {
      // Generate return statement.
      if (CanElideCallback(api)) {
        out << "::android::hardware::Return<"
            << GetCppVariableType(api.return_type_hidl(0), &message) << "> ";
      } else {
        out << "::android::hardware::Return<void> ";
      }
      // Generate function call.
      out << "Vts" << message.component_name().substr(1) << "::" << api.name()
          << "(\n";
      out.indent();
      for (int index = 0; index < api.arg_size(); index++) {
        const auto& arg = api.arg(index);
        if (isElidableType(arg.type())) {
          out << GetCppVariableType(arg, &message);
        } else {
          out << "const " << GetCppVariableType(arg, &message) << "&";
        }
        out << " arg" << index;
        if (index != (api.arg_size() - 1))
          out << ",\n";
      }
      if (api.return_type_hidl_size() == 0 || CanElideCallback(api)) {
        out << ") {" << "\n";
      } else {  // handle the case of callbacks.
        out << (api.arg_size() != 0 ? ", " : "");
        out << "std::function<void(";
        for (int index = 0; index < api.return_type_hidl_size(); index++) {
          const auto& return_val = api.return_type_hidl(index);
          if (isElidableType(return_val.type())) {
            out << GetCppVariableType(return_val, &message);
          } else {
            out << "const " << GetCppVariableType(return_val, &message) << "&";
          }
          out << " arg" << index;
          if (index != (api.return_type_hidl_size() - 1))
            out << ",";
        }
        out << ")>) {" << "\n";
      }
      out << "cout << \"" << api.name() << " called\" << endl;" << "\n";
      if (api.return_type_hidl_size() == 0
          || api.return_type_hidl(0).type() == TYPE_VOID) {
        out << "return ::android::hardware::Void();" << "\n";
      } else {
        out << "return hardware::Status::ok();" << "\n";
      }
      out.unindent();
      out << "}" << "\n";
      out << "\n";
    }

    out << "sp<" << message.component_name() << "> VtsFuzzerCreate"
        << message.component_name() << "(const string& callback_socket_name)";
    out << " {" << "\n";
    out.indent();
    out << "sp<" << message.component_name() << "> result;\n";
    out << "result = new Vts" << message.component_name().substr(1) << "();\n";
    out << "return result;\n";
    out.unindent();
    out << "}" << "\n" << "\n";
  }
}

void HalHidlCodeGen::GenerateScalarTypeInC(Formatter& out, const string& type) {
  if (type == "bool_t") {
    out << "bool";
  } else if (type == "int8_t" ||
             type == "uint8_t" ||
             type == "int16_t" ||
             type == "uint16_t" ||
             type == "int32_t" ||
             type == "uint32_t" ||
             type == "int64_t" ||
             type == "uint64_t" ||
             type == "size_t") {
    out << type;
  } else if (type == "float_t") {
    out << "float";
  } else if (type == "double_t") {
    out << "double";
  } else if (type == "char_pointer") {
    out << "char*";
  } else if (type == "void_pointer") {
    out << "void*";
  } else {
    cerr << __func__ << ":" << __LINE__
         << " unsupported scalar type " << type << "\n";
    exit(-1);
  }
}


void HalHidlCodeGen::GenerateCppBodyFuzzFunction(
    Formatter& out, const ComponentSpecificationMessage& /*message*/,
    const string& fuzzer_extended_class_name) {
    out << "bool " << fuzzer_extended_class_name << "::Fuzz(" << "\n";
    out << "    FunctionSpecificationMessage* func_msg," << "\n";
    out << "    void** result, const string& callback_socket_name) {\n";
    out.indent();
    out << "return true;\n";
    out.unindent();
    out << "}\n";
}

void HalHidlCodeGen::GenerateDriverFunctionImpl(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  if (message.component_name() != "types"
      && !endsWith(message.component_name(), "Callback")) {
    out << "bool " << fuzzer_extended_class_name << "::CallFunction("
        << "const FunctionSpecificationMessage& func_msg, "
        << "const string& callback_socket_name, "
        << "FunctionSpecificationMessage* result_msg) {\n";
    out.indent();

    out << "const char* func_name = func_msg.name().c_str();" << "\n";
    out << "cout << \"Function: \" << __func__ << \" \" << func_name << endl;"
        << "\n";

    for (auto const& api : message.interface().api()) {
      GenerateDriverImplForMethod(out, message, api);
    }

    GenerateDriverImplForReservedMethods(out);

    out << "return false;\n";
    out.unindent();
    out << "}\n";
  }
}

void HalHidlCodeGen::GenerateDriverImplForReservedMethods(Formatter& out) {
  // Generate call for reserved method: notifySyspropsChanged.
  out << "if (!strcmp(func_name, \"notifySyspropsChanged\")) {\n";
  out.indent();

  out << "cout << \"Call notifySyspropsChanged\" << endl;" << "\n";
  out << kInstanceVariableName << "->notifySyspropsChanged();\n";
  out << "result_msg->set_name(\"notifySyspropsChanged\");\n";
  out << "cout << \"called\" << endl;\n";
  out << "return true;\n";

  out.unindent();
  out << "}\n";
  // TODO(zhuoyao): Add generation code for other reserved method,
  // e.g interfaceChain
}

void HalHidlCodeGen::GenerateDriverImplForMethod(Formatter& out,
    const ComponentSpecificationMessage& message,
    const FunctionSpecificationMessage& func_msg) {
  out << "if (!strcmp(func_name, \"" << func_msg.name() << "\")) {\n";
  out.indent();
  // Process the arguments.
  for (int i = 0; i < func_msg.arg_size(); i++) {
    const auto& arg = func_msg.arg(i);
    string cur_arg_name = "arg" + std::to_string(i);
    out << GetCppVariableType(arg, &message) << " " << cur_arg_name << ";\n";
    GenerateDriverImplForTypedVariable(
        out, arg, cur_arg_name, "func_msg.arg(" + std::to_string(i) + ")");
  }

  GenerateCodeToStartMeasurement(out);
  // may need to check whether the function is actually defined.
  out << "cout << \"Call an API\" << endl;" << "\n";
  out << "cout << \"local_device = \" << " << kInstanceVariableName << ".get()"
      << " << endl;\n";

  // Define the return results and call the Hal function.
  for (int index = 0; index < func_msg.return_type_hidl_size(); index++) {
    const auto& return_type = func_msg.return_type_hidl(index);
    out << GetCppVariableType(return_type, &message) << " result" << index
        << ";\n";
  }
  if (CanElideCallback(func_msg)) {
    out << "result0 = ";
    GenerateHalFunctionCall(out, message, func_msg);
  } else {
    GenerateHalFunctionCall(out, message, func_msg);
  }

  GenerateCodeToStopMeasurement(out);

  // Set the return results value to the proto message.
  out << "result_msg->set_name(\"" << func_msg.name() << "\");\n";
  for (int index = 0; index < func_msg.return_type_hidl_size(); index++) {
    out << "VariableSpecificationMessage* result_val_" << index << " = "
        << "result_msg->add_return_type_hidl();\n";
    GenerateSetResultCodeForTypedVariable(out, func_msg.return_type_hidl(index),
                                          "result_val_" + std::to_string(index),
                                          "result" + std::to_string(index));
  }

  out << "cout << \"called\" << endl;\n";
  out << "return true;\n";
  out.unindent();
  out << "}\n";
}

void HalHidlCodeGen::GenerateHalFunctionCall(Formatter& out,
    const ComponentSpecificationMessage& message,
    const FunctionSpecificationMessage& func_msg) {
  out << kInstanceVariableName << "->" << func_msg.name() << "(";
  for (int index = 0; index < func_msg.arg_size(); index++) {
    out << "arg" << index;
    if (index != (func_msg.arg_size() - 1)) out << ",";
  }
  if (func_msg.return_type_hidl_size()== 0 || CanElideCallback(func_msg)) {
    out << ");\n";
  } else {
    out << (func_msg.arg_size() != 0 ? ", " : "");
    GenerateSyncCallbackFunctionImpl(out, message, func_msg);
    out << ");\n";
  }
}

void HalHidlCodeGen::GenerateSyncCallbackFunctionImpl(Formatter& out,
    const ComponentSpecificationMessage& message,
    const FunctionSpecificationMessage& func_msg) {
  out << "[&](";
  for (int index = 0; index < func_msg.return_type_hidl_size(); index++) {
    const auto& return_val = func_msg.return_type_hidl(index);
    if (isElidableType(return_val.type())) {
      out << GetCppVariableType(return_val, &message);
    } else {
      out << "const " << GetCppVariableType(return_val, &message) << "&";
    }
    out << " arg" << index;
    if (index != (func_msg.return_type_hidl_size() - 1)) out << ",";
  }
  out << "){\n";
  out.indent();
  out << "cout << \"callback " << func_msg.name() << " called\""
      << " << endl;\n";

  for (int index = 0; index < func_msg.return_type_hidl_size(); index++) {
    const auto& return_val = func_msg.return_type_hidl(index);
    if (return_val.type() != TYPE_FMQ_SYNC
        && return_val.type() != TYPE_FMQ_UNSYNC)
      out << "result" << index << " = arg" << index << ";\n";
  }
  out.unindent();
  out << "}";
}

void HalHidlCodeGen::GenerateCppBodyGetAttributeFunction(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  if (message.component_name() != "types" &&
      !endsWith(message.component_name(), "Callback")) {
    out << "bool " << fuzzer_extended_class_name << "::GetAttribute(" << "\n";
    out << "    FunctionSpecificationMessage* func_msg," << "\n";
    out << "    void** result) {" << "\n";

    // TOOD: impl
    cerr << __func__ << ":" << __LINE__
         << " not supported for HIDL HAL yet" << "\n";

    out << "  cerr << \"attribute not found\" << endl;" << "\n";
    out << "  return false;" << "\n";
    out << "}" << "\n";
  }
}

void HalHidlCodeGen::GenerateClassConstructionFunction(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  out << fuzzer_extended_class_name << "() : FuzzerBase(";
  if (message.component_name() != "types") {
    out << "HAL_HIDL), hw_binder_proxy_()";
  } else {
    out << "HAL_HIDL)";
  }
  out << " {}" << "\n";
}

void HalHidlCodeGen::GenerateHeaderGlobalFunctionDeclarations(Formatter& out,
    const ComponentSpecificationMessage& message) {
  if (message.component_name() != "types"
      && !endsWith(message.component_name(), "Callback")) {
    DriverCodeGenBase::GenerateHeaderGlobalFunctionDeclarations(out, message);
  }
}

void HalHidlCodeGen::GenerateCppBodyGlobalFunctions(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  if (message.component_name() != "types"
      && !endsWith(message.component_name(), "Callback")) {
    DriverCodeGenBase::GenerateCppBodyGlobalFunctions(
        out, message, fuzzer_extended_class_name);
  }
}

void HalHidlCodeGen::GenerateClassHeader(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  if (message.component_name() != "types"
      && !endsWith(message.component_name(), "Callback")) {
    DriverCodeGenBase::GenerateClassHeader(out, message,
                                           fuzzer_extended_class_name);
  } else if (message.component_name() == "types") {
    for (const auto attribute : message.attribute()) {
      GenerateDriverDeclForAttribute(out, attribute);
      if (attribute.type() == TYPE_ENUM) {
        string attribute_name = ClearStringWithNameSpaceAccess(
               attribute.name());
        out << attribute.name() << " " << "Random" << attribute_name
            << "();\n";
      }
      GenerateVerificationDeclForAttribute(out, attribute);
      GenerateSetResultDeclForAttribute(out, attribute);
    }
  } else if (endsWith(message.component_name(), "Callback")) {
    out << "\n";
    out << "class Vts" << message.component_name().substr(1) << ": public "
        << message.component_name() << " {" << "\n";
    out << " public:" << "\n";
    out.indent();
    out << "Vts" << message.component_name().substr(1) << "() {};" << "\n";
    out << "\n";
    out << "virtual ~Vts" << message.component_name().substr(1) << "()"
        << " = default;" << "\n";
    out << "\n";
    for (const auto& api : message.interface().api()) {
      // Generate return statement.
      if (CanElideCallback(api)) {
        out << "::android::hardware::Return<"
            << GetCppVariableType(api.return_type_hidl(0), &message) << "> ";
      } else {
        out << "::android::hardware::Return<void> ";
      }
      // Generate function call.
      out << api.name() << "(\n";
      out.indent();
      for (int index = 0; index < api.arg_size(); index++) {
        const auto& arg = api.arg(index);
        if (isElidableType(arg.type())) {
          out << GetCppVariableType(arg, &message);
        } else {
          out << "const " << GetCppVariableType(arg, &message) << "&";
        }
        out << " arg" << index;
        if (index != (api.arg_size() - 1))
          out << ",\n";
      }
      if (api.return_type_hidl_size() == 0 || CanElideCallback(api)) {
        out << ") override;" << "\n\n";
      } else {  // handle the case of callbacks.
        out << (api.arg_size() != 0 ? ", " : "");
        out << "std::function<void(";
        for (int index = 0; index < api.return_type_hidl_size(); index++) {
          const auto& return_val = api.return_type_hidl(index);
          if (isElidableType(return_val.type())) {
            out << GetCppVariableType(return_val, &message);
          } else {
            out << "const " << GetCppVariableType(return_val, &message) << "&";
          }
          out << " arg" << index;
          if (index != (api.return_type_hidl_size() - 1))
            out << ",";
        }
        out << ")>) override;" << "\n\n";
      }
      out.unindent();
    }
    out.unindent();
    out << "};" << "\n";
    out << "\n";

    out << "sp<" << message.component_name() << "> VtsFuzzerCreate"
        << message.component_name() << "(const string& callback_socket_name);"
        << "\n";
    out << "\n";
  }
}

void HalHidlCodeGen::GenerateClassImpl(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  if (message.component_name() != "types"
      && !endsWith(message.component_name(), "Callback")) {
    for (auto attribute : message.interface().attribute()) {
      GenerateDriverImplForAttribute(out, attribute);
      GenerateRandomFunctionForAttribute(out, attribute);
      GenerateVerificationImplForAttribute(out, attribute);
      GenerateSetResultImplForAttribute(out, attribute);
    }
    GenerateGetServiceImpl(out, message, fuzzer_extended_class_name);
    DriverCodeGenBase::GenerateClassImpl(out, message,
                                         fuzzer_extended_class_name);
  } else if (message.component_name() == "types") {
    for (auto attribute : message.attribute()) {
      GenerateDriverImplForAttribute(out, attribute);
      GenerateRandomFunctionForAttribute(out, attribute);
      GenerateVerificationImplForAttribute(out, attribute);
      GenerateSetResultImplForAttribute(out, attribute);
    }
  } else if (endsWith(message.component_name(), "Callback")) {
    GenerateCppBodyCallbackFunction(out, message, fuzzer_extended_class_name);
  }
}

void HalHidlCodeGen::GenerateHeaderIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  DriverCodeGenBase::GenerateHeaderIncludeFiles(out, message,
                                                fuzzer_extended_class_name);
  if (message.has_component_name()) {
    string package_path = message.package();
    ReplaceSubString(package_path, ".", "/");

    out << "#include <" << package_path << "/"
        << GetVersionString(message.component_type_version()) << "/"
        << message.component_name() << ".h>" << "\n";
    if (message.component_name() != "types") {
      out << "#include <" << package_path << "/"
          << GetVersionString(message.component_type_version()) << "/"
          << message.component_name() << ".h>" << "\n";
    }
    out << "#include <hidl/HidlSupport.h>" << "\n";
  }
  out << "\n\n";
}

void HalHidlCodeGen::GenerateSourceIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  DriverCodeGenBase::GenerateSourceIncludeFiles(out, message,
                                                fuzzer_extended_class_name);
  out << "#include <hidl/HidlSupport.h>\n";
  string input_vfs_file_path(input_vts_file_path_);
  if (message.has_component_name()) {
    string package_path = message.package();
    ReplaceSubString(package_path, ".", "/");
    out << "#include <" << package_path << "/"
        << GetVersionString(message.component_type_version()) << "/"
        << message.component_name() << ".h>" << "\n";
    for (const auto& import : message.import()) {
      string mutable_import = import;

      string base_filename = mutable_import.substr(
          mutable_import.find_last_of("::") + 1);
      string base_dirpath = mutable_import.substr(
          0, mutable_import.find_last_of("::") - 1);
      string base_dirpath_without_version = base_dirpath.substr(
          0, mutable_import.find_last_of("@"));
      string base_dirpath_version = base_dirpath.substr(
          mutable_import.find_last_of("@") + 1);
      ReplaceSubString(base_dirpath_without_version, ".", "/");
      base_dirpath = base_dirpath_without_version + "/" + base_dirpath_version;

      string package_path_with_version = package_path + "/" +
          GetVersionString(message.component_type_version());
      if (base_dirpath == package_path_with_version) {
        if (base_filename == "types") {
          out << "#include \""
              << input_vfs_file_path.substr(
                  0, input_vfs_file_path.find_last_of("\\/")) << "/types.vts.h\""
              << "\n";
        }
        if (base_filename != "types") {
          if (message.component_name() != base_filename) {
            out << "#include <" << package_path << "/"
                << GetVersionString(message.component_type_version()) << "/"
                << base_filename << ".h>" << "\n";
          }
          if (base_filename.substr(0, 1) == "I") {
            out << "#include \""
                << input_vfs_file_path.substr(
                    0, input_vfs_file_path.find_last_of("\\/")) << "/"
                << base_filename.substr(1, base_filename.length() - 1)
                << ".vts.h\"" << "\n";
          }
        } else if (message.component_name() != base_filename) {
          // TODO: consider restoring this when hidl packaging is fully defined.
          // cpp_ss << "#include <" << base_dirpath << base_filename << ".h>" << "\n";
          out << "#include <" << package_path << "/"
              << GetVersionString(message.component_type_version()) << "/"
              << base_filename << ".h>" << "\n";
        }
      } else {
        out << "#include <" << base_dirpath << "/"
            << base_filename << ".h>" << "\n";
      }
    }
  }
}

void HalHidlCodeGen::GenerateAdditionalFuctionDeclarations(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& /*fuzzer_extended_class_name*/) {
  if (message.component_name() != "types"
      && !endsWith(message.component_name(), "Callback")) {
    out << "bool GetService(bool get_stub, const char* service_name);"
        << "\n\n";
  }
}

void HalHidlCodeGen::GeneratePrivateMemberDeclarations(Formatter& out,
    const ComponentSpecificationMessage& message) {
  out << "sp<" << message.component_name() << "> hw_binder_proxy_;" << "\n";
}

void HalHidlCodeGen::GenerateRandomFunctionForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute) {
  // Random value generator
  if (attribute.type() == TYPE_ENUM) {
    string attribute_name = ClearStringWithNameSpaceAccess(attribute.name());
    out << attribute.name() << " " << "Random" << attribute_name << "() {"
        << "\n";
    out.indent();
    out << attribute.enum_value().scalar_type() << " choice = " << "("
        << attribute.enum_value().scalar_type() << ") " << "rand() / "
        << attribute.enum_value().enumerator().size() << ";" << "\n";
    if (attribute.enum_value().scalar_type().find("u") != 0) {
      out << "if (choice < 0) choice *= -1;" << "\n";
    }
    for (int index = 0; index < attribute.enum_value().enumerator().size();
        index++) {
      out << "if (choice == ";
      out << "(" << attribute.enum_value().scalar_type() << ") ";
      if (attribute.enum_value().scalar_type() == "int8_t") {
        out << attribute.enum_value().scalar_value(index).int8_t();
      } else if (attribute.enum_value().scalar_type() == "uint8_t") {
        out << attribute.enum_value().scalar_value(index).uint8_t();
      } else if (attribute.enum_value().scalar_type() == "int16_t") {
        out << attribute.enum_value().scalar_value(index).int16_t();
      } else if (attribute.enum_value().scalar_type() == "uint16_t") {
        out << attribute.enum_value().scalar_value(index).uint16_t();
      } else if (attribute.enum_value().scalar_type() == "int32_t") {
        out << attribute.enum_value().scalar_value(index).int32_t();
      } else if (attribute.enum_value().scalar_type() == "uint32_t") {
        out << attribute.enum_value().scalar_value(index).uint32_t();
      } else if (attribute.enum_value().scalar_type() == "int64_t") {
        out << attribute.enum_value().scalar_value(index).int64_t();
      } else if (attribute.enum_value().scalar_type() == "uint64_t") {
        out << attribute.enum_value().scalar_value(index).uint64_t();
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR unsupported enum type "
            << attribute.enum_value().scalar_type() << "\n";
        exit(-1);
      }
      out << ") return " << attribute.name() << "::"
          << attribute.enum_value().enumerator(index) << ";" << "\n";
    }
    out << "return " << attribute.name() << "::"
        << attribute.enum_value().enumerator(0) << ";" << "\n";
    out.unindent();
    out << "}" << "\n";
  }
}

void HalHidlCodeGen::GenerateDriverDeclForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute) {
  if (attribute.type() == TYPE_STRUCT || attribute.type() == TYPE_UNION) {
    // Recursively generate SetResult method implementation for all sub_types.
    for (const auto sub_struct : attribute.sub_struct()) {
      GenerateDriverDeclForAttribute(out, sub_struct);
    }
    for (const auto sub_union : attribute.sub_union()) {
      GenerateDriverDeclForAttribute(out, sub_union);
    }
    string func_name = "MessageTo"
        + ClearStringWithNameSpaceAccess(attribute.name());
    out << "void " << func_name
        << "(const VariableSpecificationMessage& var_msg, " << attribute.name()
        << "* arg);\n";
  } else if (attribute.type() == TYPE_ENUM) {
    string func_name = "EnumValue"
            + ClearStringWithNameSpaceAccess(attribute.name());
    // Message to value converter
    out << attribute.name() << " " << func_name
        << "(const ScalarDataValueMessage& arg);\n";
  } else {
    cerr << __func__ << " unsupported attribute type " << attribute.type()
         << "\n";
    exit(-1);
  }
}

void HalHidlCodeGen::GenerateDriverImplForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute) {
  switch (attribute.type()) {
    case TYPE_ENUM:
    {
      string func_name = "EnumValue"
          + ClearStringWithNameSpaceAccess(attribute.name());
      // Message to value converter
      out << attribute.name() << " " << func_name
          << "(const ScalarDataValueMessage& arg) {\n";
      out.indent();
      out << "return (" << attribute.name() << ") arg."
          << attribute.enum_value().scalar_type() << "();\n";
      out.unindent();
      out << "}" << "\n";
      break;
    }
    case TYPE_STRUCT:
    {
      // Recursively generate driver implementation method for all sub_types.
      for (const auto sub_struct : attribute.sub_struct()) {
        GenerateDriverImplForAttribute(out, sub_struct);
      }
      string func_name = "MessageTo"
          + ClearStringWithNameSpaceAccess(attribute.name());
      out << "void " << func_name
          << "(const VariableSpecificationMessage& var_msg, "
          << attribute.name() << "* arg) {" << "\n";
      out.indent();
      int struct_index = 0;
      for (const auto& struct_value : attribute.struct_value()) {
        GenerateDriverImplForTypedVariable(
            out, struct_value, "arg->" + struct_value.name(),
            "var_msg.struct_value(" + std::to_string(struct_index) + ")");
        struct_index++;
      }
      out.unindent();
      out << "}\n";
      break;
    }
    case TYPE_UNION:
    {
      // Recursively generate driver implementation method for all sub_types.
      for (const auto sub_union : attribute.sub_union()) {
        GenerateDriverImplForAttribute(out, sub_union);
      }
      string func_name = "MessageTo"
          + ClearStringWithNameSpaceAccess(attribute.name());
      out << "void " << func_name
          << "(const VariableSpecificationMessage& var_msg, "
          << attribute.name() << "* arg) {" << "\n";
      out.indent();
      int union_index = 0;
      for (const auto& union_value : attribute.union_value()) {
        out << "if (var_msg.union_value(" << union_index << ").name() == \""
            << union_value.name() << "\") {" << "\n";
        out.indent();
        GenerateDriverImplForTypedVariable(
            out, union_value, "arg->" + union_value.name(),
            "var_msg.union_value(" + std::to_string(union_index) + ")");
        union_index++;
        out.unindent();
        out << "}" << "\n";
      }
      out.unindent();
      out << "}\n";
      break;
    }
    default:
    {
      cerr << __func__ << " unsupported attribute type " << attribute.type()
           << "\n";
      exit(-1);
    }
  }
}

void HalHidlCodeGen::GenerateGetServiceImpl(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  out << "bool " << fuzzer_extended_class_name
      << "::GetService(bool get_stub, const char* service_name) {" << "\n";
  out.indent();
  out << "static bool initialized = false;" << "\n";
  out << "if (!initialized) {" << "\n";
  out.indent();
  out << "cout << \"[agent:hal] HIDL getService\" << endl;" << "\n";
  out << "if (service_name) {\n"
      << "  cout << \"  - service name: \" << service_name << endl;" << "\n"
      << "}\n";
  out << "hw_binder_proxy_ = " << message.component_name() << "::getService("
      << "service_name, get_stub);" << "\n";
  out << "cout << \"[agent:hal] hw_binder_proxy_ = \" << "
      << "hw_binder_proxy_.get() << endl;" << "\n";
  out << "initialized = true;" << "\n";
  out.unindent();
  out << "}" << "\n";
  out << "return true;" << "\n";
  out.unindent();
  out << "}" << "\n" << "\n";
}

void HalHidlCodeGen::GenerateDriverImplForTypedVariable(Formatter& out,
    const VariableSpecificationMessage& val, const string& arg_name,
    const string& arg_value_name) {
  switch (val.type()) {
    case TYPE_SCALAR:
    {
      out << arg_name << " = " << arg_value_name << ".scalar_value()."
          << val.scalar_type() << "();\n";
      break;
    }
    case TYPE_STRING:
    {
      out << arg_name << " = ::android::hardware::hidl_string("
          << arg_value_name << ".string_value().message());\n";
      break;
    }
    case TYPE_ENUM:
    {
      if (val.has_predefined_type()) {
        string func_name = "EnumValue"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << arg_name << " = " << func_name << "(" << arg_value_name
            << ".scalar_value());\n";
      } else {
        out << arg_name << " = (" << val.name() << ")" << arg_value_name << "."
            << val.enum_value().scalar_type() << "();\n";
      }
      break;
    }
    case TYPE_MASK:
    {
      out << "/* TYPE_MASK not yet supported. */\n";
      break;
    }
    case TYPE_VECTOR:
    {
      out << arg_name << ".resize(" << arg_value_name << ".vector_size());\n";
      out << "for (int i = 0; i <" << arg_value_name
          << ".vector_size(); i++) {\n";
      out.indent();
      GenerateDriverImplForTypedVariable(out, val.vector_value(0),
                                         arg_name + "[i]",
                                         arg_value_name + ".vector_value(i)");
      out.unindent();
      out << "}\n";
      break;
    }
    case TYPE_ARRAY:
    {
      out << "for (int i = 0; i < " << arg_value_name
          << ".vector_size(); i++) {\n";
      out.indent();
      GenerateDriverImplForTypedVariable(out, val.vector_value(0),
                                         arg_name + "[i]",
                                         arg_value_name + ".vector_value(i)");
      out.unindent();
      out << "}\n";
      break;
    }
    case TYPE_STRUCT:
    {
      if (val.has_predefined_type()) {
        string func_name = "MessageTo"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << func_name << "(" << arg_value_name << ", &("
            << arg_name << "));\n";
      } else {
        int struct_index = 0;
        for (const auto struct_field : val.struct_value()) {
          string struct_field_name = arg_name + "." + struct_field.name();
          string struct_field_value_name = arg_value_name + ".struct_value("
              + std::to_string(struct_index) + ")";
          GenerateDriverImplForTypedVariable(out, struct_field,
                                             struct_field_name,
                                             struct_field_value_name);
          struct_index++;
        }
      }
      break;
    }
    case TYPE_UNION:
    {
      if (val.has_predefined_type()) {
        string func_name = "MessageTo"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << func_name << "(" << arg_value_name << ", &(" << arg_name
            << "));\n";
      } else {
        int union_index = 0;
        for (const auto union_field : val.union_value()) {
          string union_field_name = arg_name + "." + union_field.name();
          string union_field_value_name = arg_value_name + ".union_value("
              + std::to_string(union_index) + ")";
          GenerateDriverImplForTypedVariable(out, union_field, union_field_name,
                                             union_field_value_name);
          union_index++;
        }
      }
      break;
    }
    case TYPE_HIDL_CALLBACK:
    {
      out << arg_name << " = VtsFuzzerCreate" << val.predefined_type()
          << "(callback_socket_name);\n";
      break;
    }
    case TYPE_HANDLE:
    {
      out << "/* ERROR: TYPE_HANDLE is not supported yet. */\n";
      break;
    }
    case TYPE_HIDL_INTERFACE:
    {
      out << "/* ERROR: TYPE_HIDL_INTERFACE is not supported yet. */\n";
      break;
    }
    case TYPE_HIDL_MEMORY:
    {
      out << "/* ERROR: TYPE_HIDL_MEMORY is not supported yet. */\n";
      break;
    }
    case TYPE_POINTER:
    {
      out << "/* ERROR: TYPE_POINTER is not supported yet. */\n";
      break;
    }
    case TYPE_FMQ_SYNC:
    {
      out << "/* ERROR: TYPE_FMQ_SYNC is not supported yet. */\n";
      break;
    }
    case TYPE_FMQ_UNSYNC:
    {
      out << "/* ERROR: TYPE_FMQ_UNSYNC is not supported yet. */\n";
      break;
    }
    default:
    {
      cerr << " ERROR: unsupported type.\n";
      exit(-1);
    }
  }
}

// TODO(zhuoyao): Verify results based on verification rules instead of perform
// an exact match.
void HalHidlCodeGen::GenerateVerificationFunctionImpl(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  if (message.component_name() != "types"
      && !endsWith(message.component_name(), "Callback")) {
    // Generate the main profiler function.
    out << "\nbool " << fuzzer_extended_class_name
        << "::VerifyResults(const FunctionSpecificationMessage& expected_result, "
        << "const FunctionSpecificationMessage& actual_result) {\n";
    out.indent();
    for (const FunctionSpecificationMessage api : message.interface().api()) {
      out << "if (!strcmp(actual_result.name().c_str(), \"" << api.name()
          << "\")) {\n";
      out.indent();
      out << "if (actual_result.return_type_hidl_size() != "
          << "expected_result.return_type_hidl_size() "
          << ") { return false; }\n";
      for (int i = 0; i < api.return_type_hidl_size(); i++) {
        std::string expected_result = "expected_result.return_type_hidl("
            + std::to_string(i) + ")";
        std::string actual_result = "actual_result.return_type_hidl("
            + std::to_string(i) + ")";
        GenerateVerificationCodeForTypedVariable(out, api.return_type_hidl(i),
                                                 expected_result,
                                                 actual_result);
      }
      out << "return true;\n";
      out.unindent();
      out << "}\n";
    }
    out << "return false;\n";
    out.unindent();
    out << "}\n\n";
  }
}

void HalHidlCodeGen::GenerateVerificationCodeForTypedVariable(Formatter& out,
    const VariableSpecificationMessage& val, const string& expected_result,
    const string& actual_result) {
  switch (val.type()) {
    case TYPE_SCALAR:
    {
      out << "if (" << actual_result << ".scalar_value()." << val.scalar_type()
          << "() != " << expected_result << ".scalar_value()."
          << val.scalar_type() << "()) { return false; }\n";
      break;
    }
    case TYPE_STRING:
    {
      out << "if (strcmp(" << actual_result
          << ".string_value().message().c_str(), " << expected_result
          << ".string_value().message().c_str())!= 0)" << "{ return false; }\n";
      break;
    }
    case TYPE_ENUM:
    {
      if (val.has_predefined_type()) {
        string func_name = "Verify"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << "if(!" << func_name << "(" << expected_result << ", "
            << actual_result << ")) { return false; }\n";
      } else {
        out << "if (" << actual_result << ".scalar_value()."
            << val.enum_value().scalar_type() << "() != " << expected_result
            << ".scalar_value()." << val.enum_value().scalar_type()
            << "()) { return false; }\n";
      }
      break;
    }
    case TYPE_MASK:
    {
      out << "/* ERROR: TYPE_MASK is not supported yet. */\n";
      break;
    }
    case TYPE_VECTOR:
    {
      out << "for (int i = 0; i <" << expected_result
          << ".vector_size(); i++) {\n";
      out.indent();
      GenerateVerificationCodeForTypedVariable(
          out, val.vector_value(0), expected_result + ".vector_value(i)",
          actual_result + ".vector_value(i)");
      out.unindent();
      out << "}\n";
      break;
    }
    case TYPE_ARRAY:
    {
      out << "for (int i = 0; i < " << expected_result
          << ".vector_size(); i++) {\n";
      out.indent();
      GenerateVerificationCodeForTypedVariable(
          out, val.vector_value(0), expected_result + ".vector_value(i)",
          actual_result + ".vector_value(i)");
      out.unindent();
      out << "}\n";
      break;
    }
    case TYPE_STRUCT:
    {
      if (val.has_predefined_type()) {
        string func_name = "Verify"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << "if (!" << func_name << "(" << expected_result << ", "
            << actual_result << ")) { return false; }\n";
      } else {
        for (int i = 0; i < val.struct_value_size(); i++) {
          string struct_field_actual_result = actual_result + ".struct_value("
              + std::to_string(i) + ")";
          string struct_field_expected_result = expected_result
              + ".struct_value(" + std::to_string(i) + ")";
          GenerateVerificationCodeForTypedVariable(out, val.struct_value(i),
                                                   struct_field_expected_result,
                                                   struct_field_actual_result);
        }
      }
      break;
    }
    case TYPE_UNION:
    {
      if (val.has_predefined_type()) {
        string func_name = "Verify"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << "if (!" << func_name << "(" << expected_result << ", "
            << actual_result << ")) {return false; }\n";
      } else {
        for (int i = 0; i < val.union_value_size(); i++) {
          string union_field_actual_result = actual_result + ".union_value("
              + std::to_string(i) + ")";
          string union_field_expected_result = expected_result + ".union_value("
              + std::to_string(i) + ")";
          GenerateVerificationCodeForTypedVariable(out, val.union_value(i),
                                                   union_field_expected_result,
                                                   union_field_actual_result);
        }
      }
      break;
    }
    case TYPE_HIDL_CALLBACK:
    {
      out << "/* ERROR: TYPE_HIDL_CALLBACK is not supported yet. */\n";
      break;
    }
    case TYPE_HANDLE:
    {
      out << "/* ERROR: TYPE_HANDLE is not supported yet. */\n";
      break;
    }
    case TYPE_HIDL_INTERFACE:
    {
      out << "/* ERROR: TYPE_HIDL_INTERFACE is not supported yet. */\n";
      break;
    }
    case TYPE_HIDL_MEMORY:
    {
      out << "/* ERROR: TYPE_HIDL_MEMORY is not supported yet. */\n";
      break;
    }
    case TYPE_POINTER:
    {
      out << "/* ERROR: TYPE_POINTER is not supported yet. */\n";
      break;
    }
    case TYPE_FMQ_SYNC:
    {
      out << "/* ERROR: TYPE_FMQ_SYNC is not supported yet. */\n";
      break;
    }
    case TYPE_FMQ_UNSYNC:
    {
      out << "/* ERROR: TYPE_FMQ_UNSYNC is not supported yet. */\n";
      break;
    }
    default:
    {
      cerr << " ERROR: unsupported type.\n";
      exit(-1);
    }
  }
}

void HalHidlCodeGen::GenerateVerificationDeclForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute) {
  if (attribute.type() == TYPE_STRUCT || attribute.type() == TYPE_UNION) {
    // Recursively generate verification method implementation for all sub_types.
    for (const auto sub_struct : attribute.sub_struct()) {
      GenerateVerificationDeclForAttribute(out, sub_struct);
    }
    for (const auto sub_union : attribute.sub_union()) {
      GenerateVerificationDeclForAttribute(out, sub_union);
    }
  }
  std::string func_name = "bool Verify"
      + ClearStringWithNameSpaceAccess(attribute.name());
  out << func_name << "(const VariableSpecificationMessage& expected_result, "
      << "const VariableSpecificationMessage& actual_result);\n";
}

void HalHidlCodeGen::GenerateVerificationImplForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute) {
  if (attribute.type() == TYPE_STRUCT || attribute.type() == TYPE_UNION) {
    // Recursively generate verification method implementation for all sub_types.
    for (const auto sub_struct : attribute.sub_struct()) {
      GenerateVerificationImplForAttribute(out, sub_struct);
    }
    for (const auto sub_union : attribute.sub_union()) {
      GenerateVerificationImplForAttribute(out, sub_union);
    }
  }
  std::string func_name = "bool Verify"
      + ClearStringWithNameSpaceAccess(attribute.name());
  out << func_name << "(const VariableSpecificationMessage& expected_result, "
      << "const VariableSpecificationMessage& actual_result){\n";
  out.indent();
  GenerateVerificationCodeForTypedVariable(out, attribute, "expected_result",
                                           "actual_result");
  out << "return true;\n";
  out.unindent();
  out << "}\n\n";
}

// TODO(zhuoyao): consider to generalize the pattern for
// Verification/SetResult/DriverImpl.
void HalHidlCodeGen::GenerateSetResultCodeForTypedVariable(Formatter& out,
    const VariableSpecificationMessage& val, const string& result_msg,
    const string& result_value) {
  switch (val.type()) {
    case TYPE_SCALAR:
    {
      out << result_msg << "->set_type(TYPE_SCALAR);\n";
      out << result_msg << "->set_scalar_type(\"" << val.scalar_type()
          << "\");\n";
      out << result_msg << "->mutable_scalar_value()->set_" << val.scalar_type()
          << "(" << result_value << ");\n";
      break;
    }
    case TYPE_STRING:
    {
      out << result_msg << "->set_type(TYPE_STRING);\n";
      out << result_msg << "->mutable_string_value()->set_message" << "("
          << result_value << ".c_str());\n";
      out << result_msg << "->mutable_string_value()->set_length" << "("
          << result_value << ".size());\n";
      break;
    }
    case TYPE_ENUM:
    {
      out << result_msg << "->set_type(TYPE_ENUM);\n";
      if (val.has_predefined_type()) {
        string func_name = "SetResult"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << func_name << "(" << result_msg << ", " << result_value << ");\n";
      } else {
        const string scalar_type = val.enum_value().scalar_type();
        out << result_msg << "->set_scalar_type(\"" << scalar_type << "\");\n";
        out << result_msg << "->mutable_scalar_value()->set_" << scalar_type
            << "(static_cast<" << scalar_type << ">(" << result_value
            << "));\n";
      }
      break;
    }
    case TYPE_MASK:
    {
      out << "/* TYPE_MASK not yet supported. */\n";
      break;
    }
    case TYPE_VECTOR:
    {
      out << result_msg << "->set_type(TYPE_VECTOR);\n";
      out << "for (int i = 0; i < (int)" << result_value << ".size(); i++) {\n";
      out.indent();
      string vector_element_name = result_msg + "_vector_i";
      out << "auto *" << vector_element_name << " = " << result_msg
          << "->add_vector_value();\n";
      GenerateSetResultCodeForTypedVariable(out, val.vector_value(0),
                                            vector_element_name,
                                            result_value + "[i]");
      out.unindent();
      out << "}\n";
      break;
    }
    case TYPE_ARRAY:
    {
      out << result_msg << "->set_type(TYPE_ARRAY);\n";
      out << "for (int i = 0; i < " << val.vector_size() << "; i++) {\n";
      out.indent();
      string array_element_name = result_msg + "_array_i";
      out << "auto *" << array_element_name << " = " << result_msg
          << "->add_vector_value();\n";
      GenerateSetResultCodeForTypedVariable(out, val.vector_value(0),
                                            array_element_name,
                                            result_value + "[i]");
      out.unindent();
      out << "}\n";
      break;
    }
    case TYPE_STRUCT:
    {
      out << result_msg << "->set_type(TYPE_STRUCT);\n";
      if (val.has_predefined_type()) {
        string func_name = "SetResult"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << func_name << "(" << result_msg << ", " << result_value << ");\n";
      } else {
        for (const auto struct_field : val.struct_value()) {
          string struct_field_name = result_msg + "_" + struct_field.name();
          out << "auto *" << struct_field_name << " = " << result_msg
              << "->add_struct_value();\n";
          GenerateSetResultCodeForTypedVariable(
              out, struct_field, struct_field_name,
              result_value + "." + struct_field.name());
          if (struct_field.has_name()) {
            out << struct_field_name << "->set_name(\""
                << struct_field.name() << "\");\n";
          }
        }
      }
      break;
    }
    case TYPE_UNION:
    {
      out << result_msg << "->set_type(TYPE_UNION);\n";
      if (val.has_predefined_type()) {
        string func_name = "SetResult"
            + ClearStringWithNameSpaceAccess(val.predefined_type());
        out << func_name << "(" << result_msg << ", " << result_value << ");\n";
      } else {
        for (const auto union_field : val.union_value()) {
          string union_field_name = result_msg + "_" + union_field.name();
          out << "auto *" << union_field_name << " = " << result_msg
              << "->add_union_value();\n";
          GenerateSetResultCodeForTypedVariable(
              out, union_field, union_field_name,
              result_value + "." + union_field.name());
        }
      }
      break;
    }
    case TYPE_HIDL_CALLBACK:
    {
      out << result_msg << "->set_type(TYPE_HIDL_CALLBACK);\n";
      out << " ERROR: TYPE_HIDL_CALLBACK is not supported yet.\n";
      break;
    }
    case TYPE_HANDLE:
    {
      out << result_msg << "->set_type(TYPE_HANDLE);\n";
      out << "/* ERROR: TYPE_HANDLE is not supported yet. */\n";
      break;
    }
    case TYPE_HIDL_INTERFACE:
    {
      out << result_msg << "->set_type(TYPE_HIDL_INTERFACE);\n";
      out << "/* ERROR: TYPE_HIDL_INTERFACE is not supported yet. */\n";
      break;
    }
    case TYPE_HIDL_MEMORY:
    {
      out << result_msg << "->set_type(TYPE_HIDL_MEMORY);\n";
      out << "/* ERROR: TYPE_HIDL_MEMORY is not supported yet. */\n";
      break;
    }
    case TYPE_POINTER:
    {
      out << result_msg << "->set_type(TYPE_POINTER);\n";
      out << "/* ERROR: TYPE_POINTER is not supported yet. */\n";
      break;
    }
    case TYPE_FMQ_SYNC:
    {
      out << result_msg << "->set_type(TYPE_FMQ_SYNC);\n";
      out << "/* ERROR: TYPE_FMQ_SYNC is not supported yet. */\n";
      break;
    }
    case TYPE_FMQ_UNSYNC:
    {
      out << result_msg << "->set_type(TYPE_FMQ_UNSYNC);\n";
      out << "/* ERROR: TYPE_FMQ_UNSYNC is not supported yet. */\n";
      break;
    }
    default:
    {
      cerr << " ERROR: unsupported type.\n";
      exit(-1);
    }
  }
}

void HalHidlCodeGen::GenerateSetResultDeclForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute) {
  if (attribute.type() == TYPE_STRUCT || attribute.type() == TYPE_UNION) {
    // Recursively generate SetResult method implementation for all sub_types.
    for (const auto sub_struct : attribute.sub_struct()) {
      GenerateSetResultDeclForAttribute(out, sub_struct);
    }
    for (const auto sub_union : attribute.sub_union()) {
      GenerateSetResultDeclForAttribute(out, sub_union);
    }
  }
  string func_name = "void SetResult"
      + ClearStringWithNameSpaceAccess(attribute.name());
  out << func_name << "(VariableSpecificationMessage* result_msg, "
      << attribute.name() << " result_value);\n";
}

void HalHidlCodeGen::GenerateSetResultImplForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute) {
  if (attribute.type() == TYPE_STRUCT || attribute.type() == TYPE_UNION) {
    // Recursively generate SetResult method implementation for all sub_types.
    for (const auto sub_struct : attribute.sub_struct()) {
      GenerateSetResultImplForAttribute(out, sub_struct);
    }
    for (const auto sub_union : attribute.sub_union()) {
      GenerateSetResultImplForAttribute(out, sub_union);
    }
  }
  string func_name = "void SetResult"
      + ClearStringWithNameSpaceAccess(attribute.name());
  out << func_name << "(VariableSpecificationMessage* result_msg, "
      << attribute.name() << " result_value){\n";
  out.indent();
  GenerateSetResultCodeForTypedVariable(out, attribute, "result_msg",
                                        "result_value");
  out.unindent();
  out << "}\n\n";
}

bool HalHidlCodeGen::CanElideCallback(
    const FunctionSpecificationMessage& func_msg) {
  // Can't elide callback for void or tuple-returning methods
  if (func_msg.return_type_hidl_size() != 1) {
    return false;
  }
  return isElidableType(func_msg.return_type_hidl(0).type());
}

bool HalHidlCodeGen::isElidableType(const VariableType& type) {
    if (type == TYPE_SCALAR || type == TYPE_ENUM || type == TYPE_MASK
        || type == TYPE_POINTER || type == TYPE_HIDL_INTERFACE
        || type == TYPE_VOID) {
        return true;
    }
    return false;
}

}  // namespace vts
}  // namespace android
