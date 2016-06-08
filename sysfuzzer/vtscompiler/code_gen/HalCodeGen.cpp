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

#include "test/vts/runners/host/proto/InterfaceSpecificationMessage.pb.h"

#include "VtsCompilerUtils.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const HalCodeGen::kInstanceVariableName = "device_";


void ReplaceSubString(string& original, const string& from, const string& to) {
  size_t index = 0;
  int from_len = from.length();
  while (true) {
    index = original.find(from, index);
    if (index == std::string::npos) break;
    original.replace(index, from_len, to);
    index += from_len;
  }
}


void HalCodeGen::GenerateCppBodyCallbackFunction(
    std::stringstream& cpp_ss,
    const InterfaceSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {

  if (message.aggregate_type_name_size()
      != message.aggregate_type_definition_size()) {
    cerr << "ERROR aggregate type's name and definition is not the equal" << endl;
    exit(-1);
  }

  for (int i = 0; i < message.aggregate_type_name_size(); i++) {
    // auto const& name = ;
    const ArgumentSpecificationMessage& definition = message.aggregate_type_definition(i);
    if (definition.is_callback()) {
      string name = "vts_callback_" + fuzzer_extended_class_name + "_" + message.aggregate_type_name(i);
      cpp_ss << "static int agent_port_ = -1;" << endl;
      cpp_ss << endl;
      cpp_ss << "class " << name << " : public FuzzerCallbackBase {" << endl;
      cpp_ss << " public:" << endl;
      cpp_ss << "  " << name << "(int agent_port) {" << endl;
      cpp_ss << "      agent_port_ = agent_port;" << endl;
      cpp_ss << "    }" << endl;
      int primitive_format_index = 0;
      for (const auto& primitive_type : definition.primitive_type()) {
        if (primitive_type == "function_pointer") {
          const string& callback_name = definition.primitive_name(
              primitive_format_index);
          // TODO: callback's return value is assumed to be 'void'.
          cpp_ss << endl;
          cpp_ss << "  static void " << callback_name << "(";
          for (int primitive_type_index = 0;
               primitive_type_index < definition.aggregate_value(
                   primitive_format_index).primitive_type_size();
               primitive_type_index++) {
            const string& var_type = definition.aggregate_value(
                primitive_format_index).primitive_type(primitive_type_index);
            if (primitive_type_index != 0) {
              cpp_ss << ", ";
            }
            if (var_type == "pointer") {
              cpp_ss << definition.aggregate_value(
                  primitive_format_index).primitive_name(primitive_type_index)
                      << " ";
            } else if (var_type == "char_pointer") {
              cpp_ss << "char* ";
            } else if (var_type == "int32") {
              cpp_ss << "int ";
            }
            cpp_ss << "arg" << primitive_type_index;
          }
          cpp_ss << ") {" << endl;
#if USE_VAARGS
          cpp_ss << "    const char fmt[] = \""
              << definition.primitive_format(primitive_format_index) << "\";"
              << endl;
          cpp_ss << "    va_list argp;" << endl;
          cpp_ss << "    const char* p;" << endl;
          cpp_ss << "    int i;" << endl;
          cpp_ss << "    char* s;" << endl;
          cpp_ss << "    char fmtbuf[256];" << endl;
          cpp_ss << endl;
          cpp_ss << "    va_start(argp, fmt);" << endl;
          cpp_ss << endl;
          cpp_ss << "    for (p = fmt; *p != '\\0'; p++) {" << endl;
          cpp_ss << "      if (*p != '%') {" << endl;
          cpp_ss << "        putchar(*p);" << endl;
          cpp_ss << "        continue;" << endl;
          cpp_ss << "      }" << endl;
          cpp_ss << "      switch (*++p) {" << endl;
          cpp_ss << "        case 'c':" << endl;
          cpp_ss << "          i = va_arg(argp, int);" << endl;
          cpp_ss << "          putchar(i);" << endl;
          cpp_ss << "          break;" << endl;
          cpp_ss << "        case 'd':" << endl;
          cpp_ss << "          i = va_arg(argp, int);" << endl;
          cpp_ss << "          s = itoa(i, fmtbuf, 10);" << endl;
          cpp_ss << "          fputs(s, stdout);" << endl;
          cpp_ss << "          break;" << endl;
          cpp_ss << "        case 's':" << endl;
          cpp_ss << "          s = va_arg(argp, char *);" << endl;
          cpp_ss << "          fputs(s, stdout);" << endl;
          cpp_ss << "          break;" << endl;
          //cpp_ss << "        case 'p':
          cpp_ss << "        case '%':" << endl;
          cpp_ss << "          putchar('%');" << endl;
          cpp_ss << "          break;" << endl;
          cpp_ss << "      }" << endl;
          cpp_ss << "    }" << endl;
          cpp_ss << "    va_end(argp);" << endl;
#endif
          // TODO: check whether bytes is set and handle properly if not.
          cpp_ss << "    RpcCallToAgent(GetCallbackID(\"" << callback_name
              << "\"), agent_port_);" << endl;
          cpp_ss << "  }" << endl;
          cpp_ss << endl;

          primitive_format_index++;
        }
      }
      cpp_ss << endl;
      cpp_ss << " private:" << endl;
      cpp_ss << "};" << endl;
      cpp_ss << endl;
    }
  }
}


void HalCodeGen::GenerateCppBodyFuzzFunction(
    std::stringstream& cpp_ss,
    const InterfaceSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  for (auto const& sub_struct : message.sub_struct()) {
    GenerateCppBodyFuzzFunction(
        cpp_ss, sub_struct, fuzzer_extended_class_name,
        message.original_data_structure_name(),
        sub_struct.is_pointer() ? "->" : ".");
  }

  cpp_ss << "bool " << fuzzer_extended_class_name << "::Fuzz(" << endl;
  cpp_ss << "    FunctionSpecificationMessage* func_msg," << endl;
  cpp_ss << "    void** result, int agent_port) {" << endl;
  cpp_ss << "  const char* func_name = func_msg->name().c_str();" << endl;
  cpp_ss << "  cout << \"Function: \" << __func__ << \" \" << func_name << endl;"
      << endl;

  // to call another function if it's for a sub_struct
  if (message.sub_struct().size() > 0) {
    cpp_ss << "  if (func_msg->parent_path().length() > 0) {" << endl;
    for (auto const& sub_struct : message.sub_struct()) {
      GenerateSubStructFuzzFunctionCall(cpp_ss, sub_struct, "");
    }
    cpp_ss << "  }" << endl;
  }

  cpp_ss << "    " << message.original_data_structure_name()
      << "* local_device = ";
  cpp_ss << "reinterpret_cast<" << message.original_data_structure_name()
      << "*>(" << kInstanceVariableName << ");" << endl;

  cpp_ss << "    if (local_device == NULL) {" << endl;
  cpp_ss << "      cout << \"use hmi\" << endl;" << endl;
  cpp_ss << "      local_device = reinterpret_cast<"
      << message.original_data_structure_name() << "*>(hmi_);" << endl;
  cpp_ss << "    }" << endl;
  cpp_ss << "    if (local_device == NULL) {" << endl;
  cpp_ss << "      cerr << \"both device_ and hmi_ are NULL.\" << endl;" << endl;
  cpp_ss << "      return false;" << endl;
  cpp_ss << "    }" << endl;

  for (auto const& api : message.api()) {
    cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg.is_callback()) {
        string name = "vts_callback_" + fuzzer_extended_class_name + "_"
            + arg.aggregate_type(arg_count);
        if (name.back() == '*') name.pop_back();
        cpp_ss << "    " << name << "* arg" << arg_count << "callback = new ";
        cpp_ss << name << "(agent_port);" << endl;
        cpp_ss << "    arg" << arg_count << "callback->Register(func_msg->arg("
            << arg_count << "));" << endl;

        cpp_ss << "    " << GetCppVariableType(arg) << " ";
        cpp_ss << "arg" << arg_count << " = (" << GetCppVariableType(arg)
            << ") malloc(sizeof(" << GetCppVariableType(arg) << "));" << endl;
        // TODO: think about how to free the malloced callback data structure.
        // find the spec.
        bool found = false;
        for (int aggregate_index = 0;
             aggregate_index < message.aggregate_type_name_size();
             aggregate_index++) {
          const ArgumentSpecificationMessage& definition =
              message.aggregate_type_definition(aggregate_index);
          if (definition.is_callback()) {
            string target_name = "vts_callback_" + fuzzer_extended_class_name
                + "_" + message.aggregate_type_name(aggregate_index);
            if (name == target_name) {
              for (int method_index = 0;
                   method_index < definition.primitive_type_size();
                   method_index++) {
                cpp_ss << "    arg" << arg_count << "->"
                    << definition.primitive_name(method_index)
                    << " = arg" << arg_count << "callback->"
                    << definition.primitive_name(method_index) << ";" << endl;
              }
              found = true;
              break;
            }
          }
        }
        if (!found) {
          cerr << __func__ << " ERROR callback's definition missing." << endl;
          exit(-1);
        }
      } else {
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
          msg_ss << "func_msg->arg(" << arg_count << ")";
          string msg = msg_ss.str();

          if (arg.primitive_type().size() > 0) {
            cpp_ss << "(" << msg << ".aggregate_type().size() == 0 && "
                << msg << ".primitive_type().size() == 1)? ";
            if (!strcmp(arg.primitive_type(0).c_str(), "pointer")
                || !strcmp(arg.primitive_type(0).c_str(), "char_pointer")
                || !strcmp(arg.primitive_type(0).c_str(), "void_pointer")
                || !strcmp(arg.primitive_type(0).c_str(), "function_pointer")) {
              cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">";
            }
            cpp_ss << "(" << msg << ".primitive_value(0)";

            if (arg.primitive_type(0) == "int32_t"
                || arg.primitive_type(0) == "uint32_t"
                || arg.primitive_type(0) == "int64_t"
                || arg.primitive_type(0) == "uint64_t"
                || arg.primitive_type(0) == "int16_t"
                || arg.primitive_type(0) == "uint16_t"
                || arg.primitive_type(0) == "int8_t"
                || arg.primitive_type(0) == "uint8_t"
                || arg.primitive_type(0) == "float_t"
                || arg.primitive_type(0) == "double_t") {
              cpp_ss << "." << arg.primitive_type(0) << "() ";
            } else if (!strcmp(arg.primitive_type(0).c_str(), "pointer")) {
              cpp_ss << ".pointer() ";
            } else if (!strcmp(arg.primitive_type(0).c_str(), "char_pointer")) {
              cpp_ss << ".pointer() ";
            } else if (!strcmp(arg.primitive_type(0).c_str(), "function_pointer")) {
              cpp_ss << ".pointer() ";
            } else if (!strcmp(arg.primitive_type(0).c_str(), "void_pointer")) {
              cpp_ss << ".pointer() ";
            } else {
              cerr << __func__ << " ERROR unsupported type " << arg.primitive_type(0) << endl;
              exit(-1);
            }
            cpp_ss << ") : ";
          }

          cpp_ss << "( (" << msg << ".aggregate_value_size() > 0 || "
              << msg << ".primitive_value_size() > 0)? ";
          cpp_ss << GetCppInstanceType(arg, msg);
          cpp_ss << " : " << GetCppInstanceType(arg) << " )";
          // TODO: use the given message and call a lib function which converts
          // a message to a C/C++ struct.
        }
        cpp_ss << ";" << endl;
      }
      cpp_ss << "    cout << \"arg" << arg_count << " = \" << arg" << arg_count
          << " << endl;" << endl;
      arg_count++;
    }

    // actual function call
    GenerateCodeToStartMeasurement(cpp_ss);
    cpp_ss << "    cout << \"hit2.\" << device_ << endl;" << endl;

    // checks whether the function is actaully defined.
    cpp_ss << "    if (reinterpret_cast<" << message.original_data_structure_name()
        << "*>(local_device)->" << api.name() << " == NULL";
    cpp_ss << ") {" << endl;
    cpp_ss << "      cerr << \"api not set.\" << endl;" << endl;
    // todo: consider throwing an exception at least a way to tell more
    // specifically to the caller.
    cpp_ss << "      return false;" << endl;
    cpp_ss << "    }" << endl;

    cpp_ss << "    cout << \"ok. let's call.\" << endl;" << endl;
    cpp_ss << "    ";
    if (api.return_type().primitive_type().size() == 1
        && !strcmp(api.return_type().primitive_type(0).c_str(), "void")) {
      cpp_ss << "*result = NULL;" << endl;
    } else {
      cpp_ss << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
    }
    cpp_ss << "local_device->" << api.name() << "(";
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
    cpp_ss << "    cout << \"called\" << endl;" << endl;

    // Copy the output (call by pointer or reference cases).
    arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg.is_output()) {
        // TODO check the return value
        cpp_ss << "    " << GetConversionToProtobufFunctionName(arg)
            << "(arg" << arg_count << ", "
            << "func_msg->mutable_arg(" << arg_count << "));" << endl;
      }
      arg_count++;
    }

    cpp_ss << "    return true;" << endl;
    cpp_ss << "  }" << endl;
  }
  // TODO: if there were pointers, free them.
  cpp_ss << "  return false;" << endl;
  cpp_ss << "}" << endl;
}


void HalCodeGen::GenerateCppBodyFuzzFunction(
    std::stringstream& cpp_ss,
    const StructSpecificationMessage& message,
    const string& fuzzer_extended_class_name,
    const string& original_data_structure_name,
    const string& parent_path) {
  for (auto const& sub_struct : message.sub_struct()) {
    GenerateCppBodyFuzzFunction(
        cpp_ss, sub_struct, fuzzer_extended_class_name,
        original_data_structure_name,
        parent_path + message.name() + (sub_struct.is_pointer() ? "->" : "."));
  }

  string parent_path_printable(parent_path);
  ReplaceSubString(parent_path_printable, "->", "_");
  replace(parent_path_printable.begin(), parent_path_printable.end(), '.', '_');

  cpp_ss << "bool " << fuzzer_extended_class_name << "::Fuzz_"
      << parent_path_printable + message.name() << "(" << endl;
  cpp_ss << "    FunctionSpecificationMessage* func_msg," << endl;
  cpp_ss << "    void** result, int agent_port) {" << endl;
  cpp_ss << "  const char* func_name = func_msg->name().c_str();" << endl;
  cpp_ss << "  cout << \"Function: \" << __func__ << \" \" << func_name << endl;" << endl;

  bool is_open;
  for (auto const& api : message.api()) {
    is_open = false;
    if ((parent_path_printable + message.name()) == "_common_methods"
        && api.name() == "open") {
      is_open = true;
    }

    cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;

    cpp_ss << "    " << original_data_structure_name
        << "* local_device = ";
    cpp_ss << "reinterpret_cast<" << original_data_structure_name
        << "*>(" << kInstanceVariableName << ");" << endl;

    cpp_ss << "    if (local_device == NULL) {" << endl;
    cpp_ss << "      cout << \"use hmi\" << endl;" << endl;
    cpp_ss << "      local_device = reinterpret_cast<" << original_data_structure_name
        << "*>(hmi_);" << endl;
    cpp_ss << "    }" << endl;
    cpp_ss << "    if (local_device == NULL) {" << endl;
    cpp_ss << "      cerr << \"both device_ and hmi_ are NULL.\" << endl;" << endl;
    cpp_ss << "      return false;" << endl;
    cpp_ss << "    }" << endl;

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      cpp_ss << "    " << GetCppVariableType(arg) << " ";
      cpp_ss << "arg" << arg_count << " = ";
      if (arg_count == 0
          && arg.aggregate_type().size() == 1
          && !strncmp(arg.aggregate_type(0).c_str(),
                      original_data_structure_name.c_str(),
                      original_data_structure_name.length())) {
        cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">("
            << kInstanceVariableName << ")";
      } else {
        std::stringstream msg_ss;
        msg_ss << "func_msg->arg(" << arg_count << ")";
        string msg = msg_ss.str();

        if (arg.primitive_type().size() > 0) {
          cpp_ss << "(" << msg << ".aggregate_type().size() == 0 && "
              << msg << ".primitive_type().size() == 1 && "
              << msg << ".primitive_value().size() == 1)? ";
          if (!strcmp(arg.primitive_type(0).c_str(), "pointer")
              || !strcmp(arg.primitive_type(0).c_str(), "char_pointer")
              || !strcmp(arg.primitive_type(0).c_str(), "void_pointer")
              || !strcmp(arg.primitive_type(0).c_str(), "function_pointer")) {
            cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">";
          }
          cpp_ss << "(" << msg << ".primitive_value(0)";

          if (arg.primitive_type(0) == "int32_t"
              || arg.primitive_type(0) == "uint32_t"
              || arg.primitive_type(0) == "int64_t"
              || arg.primitive_type(0) == "uint64_t"
              || arg.primitive_type(0) == "int16_t"
              || arg.primitive_type(0) == "uint16_t"
              || arg.primitive_type(0) == "int8_t"
              || arg.primitive_type(0) == "uint8_t"
              || arg.primitive_type(0) == "float_t"
              || arg.primitive_type(0) == "double_t") {
            cpp_ss << "." << arg.primitive_type(0) << "() ";
          } else if (!strcmp(arg.primitive_type(0).c_str(), "pointer")) {
            cpp_ss << ".pointer() ";
          } else if (!strcmp(arg.primitive_type(0).c_str(), "char_pointer")) {
            cpp_ss << ".pointer() ";
          } else if (!strcmp(arg.primitive_type(0).c_str(), "function_pointer")) {
            cpp_ss << ".pointer() ";
          } else if (!strcmp(arg.primitive_type(0).c_str(), "void_pointer")) {
            cpp_ss << ".pointer() ";
          } else {
            cerr << __func__ << " ERROR unsupported type " << arg.primitive_type(0) << endl;
            exit(-1);
          }
          cpp_ss << ") : ";
        }

        if (is_open) {
          if (arg_count == 0) {
            cpp_ss << "hmi_;" << endl;
          } else if (arg_count == 1) {
            cpp_ss << "((hmi_) ? const_cast<char*>(hmi_->name) : NULL)" << endl;
          } else if (arg_count == 2) {
            cpp_ss << "(struct hw_device_t**) &device_" << endl;
          } else {
            cerr << __func__ << " ERROR additional args for open " << arg_count << endl;
            exit(-1);
          }
        } else {
          cpp_ss << "( (" << msg << ".aggregate_value_size() > 0 || "
              << msg << ".primitive_value_size() > 0)? ";
          cpp_ss << GetCppInstanceType(arg, msg);
          cpp_ss << " : " << GetCppInstanceType(arg) << " )";
          // TODO: use the given message and call a lib function which converts
          // a message to a C/C++ struct.
        }
      }
      cpp_ss << ";" << endl;
      cpp_ss << "    cout << \"arg" << arg_count << " = \" << arg" << arg_count
          << " << endl;" << endl;
      arg_count++;
    }

    // actual function call
    GenerateCodeToStartMeasurement(cpp_ss);
    cpp_ss << "    cout << \"hit2.\" << device_ << endl;" << endl;

    cpp_ss << "    if (reinterpret_cast<" << original_data_structure_name
        << "*>(local_device)" << parent_path
        << message.name() << "->" << api.name() << " == NULL";
    cpp_ss << ") {" << endl;
    cpp_ss << "      cerr << \"api not set.\" << endl;" << endl;
    // todo: consider throwing an exception at least a way to tell more
    // specifically to the caller.
    cpp_ss << "      return false;" << endl;
    cpp_ss << "    }" << endl;

    cpp_ss << "    cout << \"ok. let's call.\" << endl;" << endl;
    cpp_ss << "    ";
    if (api.return_type().primitive_type().size() == 1
        && !strcmp(api.return_type().primitive_type(0).c_str(), "void")) {
      cpp_ss << "*result = NULL;" << endl;
    } else {
      cpp_ss << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
    }
    cpp_ss << "local_device" << parent_path
        << message.name() << "->" << api.name() << "(";
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
    cpp_ss << "    cout << \"called\" << endl;" << endl;

    // Copy the output (call by pointer or reference cases).
    arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg.is_output()) {
        // TODO check the return value
        cpp_ss << "    " << GetConversionToProtobufFunctionName(arg)
            << "(arg" << arg_count << ", "
            << "func_msg->mutable_arg(" << arg_count << "));" << endl;
      }
      arg_count++;
    }

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


void HalCodeGen::GenerateSubStructFuzzFunctionCall(
    std::stringstream& cpp_ss,
    const StructSpecificationMessage& message,
    const string& parent_path) {
  string current_path(parent_path);
  if (current_path.length() > 0) {
    current_path += ".";
  }
  current_path += message.name();

  string current_path_printable(current_path);
  replace(current_path_printable.begin(), current_path_printable.end(), '.', '_');

  cpp_ss << "    if (func_msg->parent_path() == \"" << current_path << "\") {" << endl;
  cpp_ss << "      return Fuzz__" << current_path_printable << "(func_msg, result, agent_port);"<< endl;
  cpp_ss << "    }" << endl;

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateSubStructFuzzFunctionCall(
        cpp_ss, sub_struct, current_path);
  }
}

}  // namespace vts
}  // namespace android
