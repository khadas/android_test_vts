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

#include "test/vts/proto/InterfaceSpecificationMessage.pb.h"

#include "VtsCompilerUtils.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const HalCodeGen::kInstanceVariableName = "device_";


void HalCodeGen::GenerateCppBodyCallbackFunction(
    std::stringstream& cpp_ss, const InterfaceSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  bool first_callback = true;

  for (int i = 0; i < message.attribute_size(); i++) {
    const VariableSpecificationMessage& attribute = message.attribute(i);
    if (attribute.type() != TYPE_FUNCTION_POINTER || !attribute.is_callback()) {
      continue;
    }
    string name =
        "vts_callback_" + fuzzer_extended_class_name + "_" + attribute.name();
    if (first_callback) {
      cpp_ss << "static string callback_socket_name_;" << endl;
      first_callback = false;
    }
    cpp_ss << endl;
    cpp_ss << "class " << name << " : public FuzzerCallbackBase {" << endl;
    cpp_ss << " public:" << endl;
    cpp_ss << "  " << name << "(const string& callback_socket_name) {" << endl;
    cpp_ss << "      callback_socket_name_ = callback_socket_name;" << endl;
    cpp_ss << "    }" << endl;

    int primitive_format_index = 0;
    for (const FunctionPointerSpecificationMessage& func_pt_spec :
         attribute.function_pointer()) {
      const string& callback_name = func_pt_spec.function_name();
      // TODO: callback's return value is assumed to be 'void'.
      cpp_ss << endl;
      cpp_ss << "  static ";
      bool has_return_value = false;
      if (!func_pt_spec.has_return_type() ||
          !func_pt_spec.return_type().has_type() ||
          func_pt_spec.return_type().type() == TYPE_VOID) {
        cpp_ss << "void" << endl;
      } else if (func_pt_spec.return_type().type() == TYPE_PREDEFINED) {
        cpp_ss << func_pt_spec.return_type().predefined_type();
        has_return_value = true;
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR unknown type "
             << func_pt_spec.return_type().type() << endl;
        exit(-1);
      }
      cpp_ss << " " << callback_name << "(";
      int primitive_type_index;
      primitive_type_index = 0;
      for (const auto& arg : func_pt_spec.arg()) {
        if (primitive_type_index != 0) {
          cpp_ss << ", ";
        }
        if (arg.is_const()) {
          cpp_ss << "const ";
        }
        if (arg.type() == TYPE_SCALAR) {
          /*
          if (arg.scalar_type() == "pointer") {
            cpp_ss << definition.aggregate_value(
                primitive_format_index).primitive_name(primitive_type_index)
                    << " ";
          } */
          if (arg.scalar_type() == "char_pointer") {
            cpp_ss << "char* ";
          } else if (arg.scalar_type() == "uchar_pointer") {
            cpp_ss << "unsigned char* ";
          } else if (arg.scalar_type() == "bool_t") {
            cpp_ss << "bool ";
          } else if (arg.scalar_type() == "int8_t" ||
                     arg.scalar_type() == "uint8_t" ||
                     arg.scalar_type() == "int16_t" ||
                     arg.scalar_type() == "uint16_t" ||
                     arg.scalar_type() == "int32_t" ||
                     arg.scalar_type() == "uint32_t" ||
                     arg.scalar_type() == "size_t" ||
                     arg.scalar_type() == "int64_t" ||
                     arg.scalar_type() == "uint64_t") {
            cpp_ss << arg.scalar_type() << " ";
          } else if (arg.scalar_type() == "void_pointer") {
            cpp_ss << "void*";
          } else {
            cerr << __func__ << " unsupported scalar type " << arg.scalar_type()
                 << endl;
            exit(-1);
          }
        } else if (arg.type() == TYPE_PREDEFINED) {
          cpp_ss << arg.predefined_type() << " ";
        } else {
          cerr << __func__ << " unsupported type" << endl;
          exit(-1);
        }
        cpp_ss << "arg" << primitive_type_index;
        primitive_type_index++;
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
      // cpp_ss << "        case 'p':
      cpp_ss << "        case '%':" << endl;
      cpp_ss << "          putchar('%');" << endl;
      cpp_ss << "          break;" << endl;
      cpp_ss << "      }" << endl;
      cpp_ss << "    }" << endl;
      cpp_ss << "    va_end(argp);" << endl;
#endif
      // TODO: check whether bytes is set and handle properly if not.
      cpp_ss << "    AndroidSystemCallbackRequestMessage callback_message;"
             << endl;
      cpp_ss << "    callback_message.set_id(GetCallbackID(\"" << callback_name
             << "\"));" << endl;

      primitive_type_index = 0;
      for (const auto& arg : func_pt_spec.arg()) {
        cpp_ss << "VariableSpecificationMessage* var_msg" << primitive_type_index
               << " = callback_message.add_arg();" << endl;
        if (arg.type() == TYPE_SCALAR) {
          cpp_ss << "var_msg" << primitive_type_index << "->set_type("
                 << "TYPE_SCALAR);" << endl;
          cpp_ss << "var_msg" << primitive_type_index << "->set_scalar_type(\""
                 << arg.scalar_type() << "\");" << endl;
          cpp_ss << "var_msg" << primitive_type_index << "->mutable_scalar_value()";
          if (arg.scalar_type() == "bool_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().bool_t() << ");" << endl;
          } else if (arg.scalar_type() == "int8_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().int8_t() << ");" << endl;
          } else if (arg.scalar_type() == "uint8_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().uint8_t() << ");" << endl;
          } else if (arg.scalar_type() == "int16_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().int16_t() << ");" << endl;
          } else if (arg.scalar_type() == "uint16_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().uint16_t() << ");" << endl;
          } else if (arg.scalar_type() == "int32_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().int32_t() << ");" << endl;
          } else if (arg.scalar_type() == "uint32_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().uint32_t() << ");" << endl;
          } else if (arg.scalar_type() == "size_t") {
            cpp_ss << "->set_uint32_t("
                   << arg.scalar_value().uint32_t() << ");" << endl;
          } else if (arg.scalar_type() == "int64_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().int64_t() << ");" << endl;
          } else if (arg.scalar_type() == "uint64_t") {
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().uint64_t() << ");" << endl;
          } else if (arg.scalar_type() == "char_pointer") {
            // pointer value is not meaning when it is passed to another machine.
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().char_pointer() << ");" << endl;
          } else if (arg.scalar_type() == "uchar_pointer") {
            // pointer value is not meaning when it is passed to another machine.
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().uchar_pointer() << ");" << endl;
          } else if (arg.scalar_type() == "void_pointer") {
            // pointer value is not meaning when it is passed to another machine.
            cpp_ss << "->set_" << arg.scalar_type() << "("
                   << arg.scalar_value().void_pointer() << ");" << endl;
          } else {
            cerr << __func__ << " unsupported scalar type " << arg.scalar_type()
                 << endl;
            exit(-1);
          }
        } else if (arg.type() == TYPE_PREDEFINED) {
          cpp_ss << "var_msg" << primitive_type_index << "->set_type("
                 << "TYPE_PREDEFINED);" << endl;
          // TODO: actually handle such case.
        } else {
          cerr << __func__ << " unsupported type" << endl;
          exit(-1);
        }
        primitive_type_index++;
      }
      cpp_ss << "    RpcCallToAgent(callback_message, callback_socket_name_);"
             << endl;
      if (has_return_value) {
        // TODO: consider actual return type.
        cpp_ss << "    return NULL;";
      }
      cpp_ss << "  }" << endl;
      cpp_ss << endl;

      primitive_format_index++;
    }
    cpp_ss << endl;
    cpp_ss << " private:" << endl;
    cpp_ss << "};" << endl;
    cpp_ss << endl;
  }
}

void HalCodeGen::GenerateCppBodyFuzzFunction(
    std::stringstream& cpp_ss, const InterfaceSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  for (auto const& sub_struct : message.sub_struct()) {
    GenerateCppBodyFuzzFunction(cpp_ss, sub_struct, fuzzer_extended_class_name,
                                message.original_data_structure_name(),
                                sub_struct.is_pointer() ? "->" : ".");
  }

  cpp_ss << "bool " << fuzzer_extended_class_name << "::Fuzz(" << endl;
  cpp_ss << "    FunctionSpecificationMessage* func_msg," << endl;
  cpp_ss << "    void** result, const string& callback_socket_name) {" << endl;
  cpp_ss << "  const char* func_name = func_msg->name().c_str();" << endl;
  cpp_ss
      << "  cout << \"Function: \" << __func__ << \" '\" << func_name << \"'\" << endl;"
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
  cpp_ss << "      cout << \"use hmi \" << (uint64_t)hmi_ << endl;" << endl;
  cpp_ss << "      local_device = reinterpret_cast<"
         << message.original_data_structure_name() << "*>(hmi_);" << endl;
  cpp_ss << "    }" << endl;
  cpp_ss << "    if (local_device == NULL) {" << endl;
  cpp_ss << "      cerr << \"both device_ and hmi_ are NULL.\" << endl;"
         << endl;
  cpp_ss << "      return false;" << endl;
  cpp_ss << "    }" << endl;

  for (auto const& api : message.api()) {
    cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;
    cpp_ss << "    cout << \"match\" << endl;" << endl;
    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg.is_callback()) {  // arg.type() isn't always TYPE_FUNCTION_POINTER
        string name = "vts_callback_" + fuzzer_extended_class_name + "_" +
                      arg.predefined_type();  // TODO - check to make sure name
                                              // is always correct
        if (name.back() == '*') name.pop_back();
        cpp_ss << "    " << name << "* arg" << arg_count << "callback = new ";
        cpp_ss << name << "(callback_socket_name);" << endl;
        cpp_ss << "    arg" << arg_count << "callback->Register(func_msg->arg("
               << arg_count << "));" << endl;

        cpp_ss << "    " << GetCppVariableType(arg) << " ";
        cpp_ss << "arg" << arg_count << " = (" << GetCppVariableType(arg)
               << ") malloc(sizeof(" << GetCppVariableType(arg) << "));"
               << endl;
        // TODO: think about how to free the malloced callback data structure.
        // find the spec.
        bool found = false;
        cout << name << endl;
        for (auto const& attribute : message.attribute()) {
          if (attribute.type() == TYPE_FUNCTION_POINTER &&
              attribute.is_callback()) {
            string target_name = "vts_callback_" + fuzzer_extended_class_name +
                                 "_" + attribute.name();
            cout << "compare" << endl;
            cout << target_name << endl;
            if (name == target_name) {
              if (attribute.function_pointer_size() > 1) {
                for (auto const& func_pt : attribute.function_pointer()) {
                  cpp_ss << "    arg" << arg_count << "->"
                         << func_pt.function_name() << " = arg" << arg_count
                         << "callback->" << func_pt.function_name() << ";"
                         << endl;
                }
              } else {
                cpp_ss << "    arg" << arg_count << " = arg" << arg_count
                       << "callback->" << attribute.name() << ";" << endl;
              }
              found = true;
              break;
            }
          }
        }
        if (!found) {
          cerr << __func__ << " ERROR callback definition missing for " << name
               << " of " << api.name() << endl;
          exit(-1);
        }
      } else {
        cpp_ss << "    " << GetCppVariableType(arg) << " ";
        cpp_ss << "arg" << arg_count << " = ";
        if (arg_count == 0 && arg.type() == TYPE_PREDEFINED &&
            !strncmp(arg.predefined_type().c_str(),
                     message.original_data_structure_name().c_str(),
                     message.original_data_structure_name().length())) {
          cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">("
                 << kInstanceVariableName << ")";
        } else {
          std::stringstream msg_ss;
          msg_ss << "func_msg->arg(" << arg_count << ")";
          string msg = msg_ss.str();

          if (arg.type() == TYPE_SCALAR) {
            cpp_ss << "(" << msg << ".type() == TYPE_SCALAR)? ";
            if (arg.scalar_type() == "pointer" ||
                arg.scalar_type() == "pointer_pointer" ||
                arg.scalar_type() == "char_pointer" ||
                arg.scalar_type() == "uchar_pointer" ||
                arg.scalar_type() == "void_pointer" ||
                arg.scalar_type() == "function_pointer") {
              cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">";
            }
            cpp_ss << "(" << msg << ".scalar_value()";

            if (arg.scalar_type() == "bool_t" ||
                arg.scalar_type() == "int32_t" ||
                arg.scalar_type() == "uint32_t" ||
                arg.scalar_type() == "int64_t" ||
                arg.scalar_type() == "uint64_t" ||
                arg.scalar_type() == "int16_t" ||
                arg.scalar_type() == "uint16_t" ||
                arg.scalar_type() == "int8_t" ||
                arg.scalar_type() == "uint8_t" ||
                arg.scalar_type() == "float_t" ||
                arg.scalar_type() == "double_t") {
              cpp_ss << "." << arg.scalar_type() << "() ";
            } else if (arg.scalar_type() == "pointer" ||
                       arg.scalar_type() == "char_pointer" ||
                       arg.scalar_type() == "uchar_pointer" ||
                       arg.scalar_type() == "void_pointer") {
              cpp_ss << ".pointer() ";
            } else {
              cerr << __func__ << " ERROR unsupported scalar type "
                   << arg.scalar_type() << endl;
              exit(-1);
            }
            cpp_ss << ") : ";
          } else {
            cerr << __func__ << " unknown type " << msg << endl;
          }

          cpp_ss << "( (" << msg << ".type() == TYPE_PREDEFINED || " << msg
                 << ".type() == TYPE_STRUCT || " << msg
                 << ".type() == TYPE_SCALAR)? ";
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

    // checks whether the function is actually defined.
    cpp_ss << "    if (reinterpret_cast<"
           << message.original_data_structure_name() << "*>(local_device)->"
           << api.name() << " == NULL";
    cpp_ss << ") {" << endl;
    cpp_ss << "      cerr << \"api not set.\" << endl;" << endl;
    // todo: consider throwing an exception at least a way to tell more
    // specifically to the caller.
    cpp_ss << "      return false;" << endl;
    cpp_ss << "    }" << endl;

    cpp_ss << "    cout << \"ok. let's call.\" << endl;" << endl;
    cpp_ss << "    ";
    if (!api.has_return_type() || api.return_type().type() == TYPE_VOID) {
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

    if (api.has_return_type() && api.return_type().type() != TYPE_VOID) {
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
        cpp_ss << "    " << GetConversionToProtobufFunctionName(arg) << "(arg"
               << arg_count << ", "
               << "func_msg->mutable_arg(" << arg_count << "));" << endl;
      }
      arg_count++;
    }

    cpp_ss << "    return true;" << endl;
    cpp_ss << "  }" << endl;
  }
  // TODO: if there were pointers, free them.
  cpp_ss << "  cerr << \"func not found\" << endl;" << endl;
  cpp_ss << "  return false;" << endl;
  cpp_ss << "}" << endl;
}

void HalCodeGen::GenerateCppBodyFuzzFunction(
    std::stringstream& cpp_ss, const StructSpecificationMessage& message,
    const string& fuzzer_extended_class_name,
    const string& original_data_structure_name, const string& parent_path) {
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
  cpp_ss << "    void** result, const string& callback_socket_name) {" << endl;
  cpp_ss << "  const char* func_name = func_msg->name().c_str();" << endl;
  cpp_ss
      << "  cout << \"Function: \" << __func__ << \" \" << func_name << endl;"
      << endl;

  bool is_open;
  for (auto const& api : message.api()) {
    is_open = false;
    if ((parent_path_printable + message.name()) == "_common_methods" &&
        api.name() == "open") {
      is_open = true;
    }

    cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;

    cpp_ss << "    " << original_data_structure_name << "* local_device = ";
    cpp_ss << "reinterpret_cast<" << original_data_structure_name << "*>("
           << kInstanceVariableName << ");" << endl;

    cpp_ss << "    if (local_device == NULL) {" << endl;
    cpp_ss << "      cout << \"use hmi\" << endl;" << endl;
    cpp_ss << "      local_device = reinterpret_cast<"
           << original_data_structure_name << "*>(hmi_);" << endl;
    cpp_ss << "    }" << endl;
    cpp_ss << "    if (local_device == NULL) {" << endl;
    cpp_ss << "      cerr << \"both device_ and hmi_ are NULL.\" << endl;"
           << endl;
    cpp_ss << "      return false;" << endl;
    cpp_ss << "    }" << endl;

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      cpp_ss << "    " << GetCppVariableType(arg) << " ";
      cpp_ss << "arg" << arg_count << " = ";
      if (arg_count == 0 && arg.type() == TYPE_PREDEFINED &&
          !strncmp(arg.predefined_type().c_str(),
                   original_data_structure_name.c_str(),
                   original_data_structure_name.length())) {
        cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">("
               << kInstanceVariableName << ")";
      } else {
        std::stringstream msg_ss;
        msg_ss << "func_msg->arg(" << arg_count << ")";
        string msg = msg_ss.str();

        if (arg.type() == TYPE_SCALAR) {
          cpp_ss << "(" << msg << ".type() == TYPE_SCALAR && " << msg
                 << ".scalar_value()";
          if (arg.scalar_type() == "pointer" ||
              arg.scalar_type() == "char_pointer" ||
              arg.scalar_type() == "uchar_pointer" ||
              arg.scalar_type() == "void_pointer" ||
              arg.scalar_type() == "function_pointer") {
            cpp_ss << ".has_pointer())? ";
            cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">";
          } else {
            cpp_ss << ".has_" << arg.scalar_type() << "())? ";
          }
          cpp_ss << "(" << msg << ".scalar_value()";

          if (arg.scalar_type() == "int32_t" ||
              arg.scalar_type() == "uint32_t" ||
              arg.scalar_type() == "int64_t" ||
              arg.scalar_type() == "uint64_t" ||
              arg.scalar_type() == "int16_t" ||
              arg.scalar_type() == "uint16_t" ||
              arg.scalar_type() == "int8_t" || arg.scalar_type() == "uint8_t" ||
              arg.scalar_type() == "float_t" ||
              arg.scalar_type() == "double_t") {
            cpp_ss << "." << arg.scalar_type() << "() ";
          } else if (arg.scalar_type() == "pointer" ||
                     arg.scalar_type() == "char_pointer" ||
                     arg.scalar_type() == "uchar_pointer" ||
                     arg.scalar_type() == "function_pointer" ||
                     arg.scalar_type() == "void_pointer") {
            cpp_ss << ".pointer() ";
          } else {
            cerr << __func__ << " ERROR unsupported type " << arg.scalar_type()
                 << endl;
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
            cerr << __func__ << " ERROR additional args for open " << arg_count
                 << endl;
            exit(-1);
          }
        } else {
          cpp_ss << "( (" << msg << ".type() == TYPE_PREDEFINED || " << msg
                 << ".type() == TYPE_STRUCT || " << msg
                 << ".type() == TYPE_SCALAR)? ";
          cpp_ss << GetCppInstanceType(arg, msg);
          cpp_ss << " : " << GetCppInstanceType(arg) << " )";
          // TODO: use the given message and call a lib function which converts
          // a message to a C/C++ struct.
        }
      }
      cpp_ss << ";" << endl;
      cpp_ss << "    cout << \"arg" << arg_count << " = \" << arg" << arg_count
             << " << endl;" << endl
             << endl;
      arg_count++;
    }

    // actual function call
    GenerateCodeToStartMeasurement(cpp_ss);
    cpp_ss << "    cout << \"hit2.\" << device_ << endl;" << endl;

    cpp_ss << "    if (reinterpret_cast<" << original_data_structure_name
           << "*>(local_device)" << parent_path << message.name() << "->"
           << api.name() << " == NULL";
    cpp_ss << ") {" << endl;
    cpp_ss << "      cerr << \"api not set.\" << endl;" << endl;
    // todo: consider throwing an exception at least a way to tell more
    // specifically to the caller.
    cpp_ss << "      return false;" << endl;
    cpp_ss << "    }" << endl;

    cpp_ss << "    cout << \"ok. let's call.\" << endl;" << endl;
    cpp_ss << "    ";
    if (!api.has_return_type() || api.return_type().type() == TYPE_VOID) {
      cpp_ss << "*result = NULL;" << endl;
    } else {
      cpp_ss << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
    }
    cpp_ss << "local_device" << parent_path << message.name() << "->"
           << api.name() << "(";
    if (arg_count > 0) cpp_ss << endl;

    for (int index = 0; index < arg_count; index++) {
      cpp_ss << "      arg" << index;
      if (index != (arg_count - 1)) {
        cpp_ss << "," << endl;
      }
    }
    if (api.has_return_type() && api.return_type().type() != TYPE_VOID) {
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
        cpp_ss << "    " << GetConversionToProtobufFunctionName(arg) << "(arg"
               << arg_count << ", "
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
    std::stringstream& h_ss, const string& function_prototype) {
  h_ss << "extern \"C\" {" << endl;
  h_ss << "extern " << function_prototype << ";" << endl;
  h_ss << "}" << endl;
}

void HalCodeGen::GenerateCppBodyGlobalFunctions(
    std::stringstream& cpp_ss, const string& function_prototype,
    const string& fuzzer_extended_class_name) {
  cpp_ss << "extern \"C\" {" << endl;
  cpp_ss << function_prototype << " {" << endl;
  cpp_ss << "  return (android::vts::FuzzerBase*) "
         << "new android::vts::" << fuzzer_extended_class_name << "();" << endl;
  cpp_ss << "}" << endl << endl;
  cpp_ss << "}" << endl;
}

void HalCodeGen::GenerateSubStructFuzzFunctionCall(
    std::stringstream& cpp_ss, const StructSpecificationMessage& message,
    const string& parent_path) {
  string current_path(parent_path);
  if (current_path.length() > 0) {
    current_path += ".";
  }
  current_path += message.name();

  string current_path_printable(current_path);
  replace(current_path_printable.begin(), current_path_printable.end(), '.',
          '_');

  cpp_ss << "    if (func_msg->parent_path() == \"" << current_path << "\") {"
         << endl;
  cpp_ss << "      return Fuzz__" << current_path_printable
         << "(func_msg, result, callback_socket_name);" << endl;
  cpp_ss << "    }" << endl;

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateSubStructFuzzFunctionCall(cpp_ss, sub_struct, current_path);
  }
}

}  // namespace vts
}  // namespace android
