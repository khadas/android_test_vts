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
#include "code_gen/driver/HalCodeGen.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const HalHidlCodeGen::kInstanceVariableName = "hw_binder_proxy_";

void HalHidlCodeGen::GenerateCppBodyCallbackFunction(
    std::stringstream& cpp_ss, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  bool first_callback = true;

  for (int i = 0;
       i < message.attribute_size() + message.interface().attribute_size();
       i++) {
    const VariableSpecificationMessage& attribute = (i < message.attribute_size()) ?
        message.attribute(i) :
        message.interface().attribute(i - message.attribute_size());
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
          }
          */
          if (arg.scalar_type() == "char_pointer") {
            cpp_ss << "char* ";
          } else if (arg.scalar_type() == "int32_t" ||
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


void HalHidlCodeGen::GenerateCppBodySyncCallbackFunction(
    std::stringstream& cpp_ss, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {

  for (auto const& api : message.interface().api()) {
    if (api.return_type_hidl_size() > 0) {
      cpp_ss << "static void " << fuzzer_extended_class_name << api.name() << "_cb_func(";
      if (api.return_type_hidl(0).type() == TYPE_SCALAR) {
        cpp_ss << api.return_type_hidl(0).scalar_type();
      } else if (api.return_type_hidl(0).type() == TYPE_VECTOR) {
        cpp_ss << GetCppVariableType(api.return_type_hidl(0), &message);
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
             << api.return_type_hidl(0).type() << endl;
      }
      cpp_ss << " arg) {" << endl;
      // TODO: support other non-scalar type and multiple args.
      cpp_ss << "  cout << \"callback " << api.name() << " called\""
             << " << endl;" << endl;
      cpp_ss << "}" << endl;
      cpp_ss << "std::function<"
              << "void(";
      if (api.return_type_hidl(0).type() == TYPE_SCALAR) {
        cpp_ss << api.return_type_hidl(0).scalar_type();
      } else if (api.return_type_hidl(0).type() == TYPE_VECTOR) {
        cpp_ss << GetCppVariableType(api.return_type_hidl(0), &message);
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
             << api.return_type_hidl(0).type() << endl;
      }
      cpp_ss << ")> "
             << fuzzer_extended_class_name << api.name() << "_cb = "
             << fuzzer_extended_class_name << api.name() << "_cb_func;" << endl;
      cpp_ss << endl << endl;
    }
  }
}


void HalHidlCodeGen::GenerateCppBodyFuzzFunction(
    std::stringstream& cpp_ss, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  GenerateCppBodySyncCallbackFunction(
      cpp_ss, message, fuzzer_extended_class_name);

  set<string> callbacks;
  for (auto const& api : message.interface().api()) {
    for (auto const& arg : api.arg()) {
      if (!arg.is_callback() && arg.type() == TYPE_HIDL_CALLBACK &&
          callbacks.find(arg.predefined_type()) == callbacks.end()) {
        cpp_ss << "extern " << arg.predefined_type() << "* "
               << "VtsFuzzerCreate" << arg.predefined_type()
               << "(const string& callback_socket_name);" << endl << endl;
        callbacks.insert(arg.predefined_type());
      }
    }
  }

  if (message.component_name() != "types" &&
      !endsWith(message.component_name(), "Callback")) {
    for (auto const& sub_struct : message.interface().sub_struct()) {
      GenerateCppBodyFuzzFunction(cpp_ss, sub_struct, fuzzer_extended_class_name,
                                  message.original_data_structure_name(),
                                  sub_struct.is_pointer() ? "->" : ".");
    }

    cpp_ss << "bool " << fuzzer_extended_class_name << "::GetService() {" << endl;

    cpp_ss << "  static bool initialized = false;" << endl;
    cpp_ss << "  if (!initialized) {" << endl;
    cpp_ss << "    cout << \"[agent:hal] HIDL getService\" << endl;" << endl;
    string service_name = message.package().substr(message.package().find_last_of(".") + 1);
    if (service_name == "nfc") {
      // TODO(yim): remove this special case after b/32158398 is fixed.
      service_name = "nfc_nci";
    }
    cpp_ss << "    hw_binder_proxy_ = " << message.component_name()
           << "::getService(\""
           << service_name
           << "\", true);" << endl;
    cpp_ss << "    cout << \"[agent:hal] hw_binder_proxy_ = \" << "
           << "hw_binder_proxy_.get() << endl;" << endl;
    cpp_ss << "    initialized = true;" << endl;
    cpp_ss << "  }" << endl;
    cpp_ss << "  return true;" << endl;
    cpp_ss << "}" << endl << endl;

    cpp_ss << "bool " << fuzzer_extended_class_name << "::Fuzz(" << endl;
    cpp_ss << "    FunctionSpecificationMessage* func_msg," << endl;
    cpp_ss << "    void** result, const string& callback_socket_name) {" << endl;

    cpp_ss << "  const char* func_name = func_msg->name().c_str();" << endl;
    cpp_ss
        << "  cout << \"Function: \" << __func__ << \" \" << func_name << endl;"
        << endl;

    // to call another function if it's for a sub_struct
    if (message.interface().sub_struct().size() > 0) {
      cpp_ss << "  if (func_msg->parent_path().length() > 0) {" << endl;
      for (auto const& sub_struct : message.interface().sub_struct()) {
        GenerateSubStructFuzzFunctionCall(cpp_ss, sub_struct, "");
      }
      cpp_ss << "  }" << endl;
    }

    for (auto const& api : message.interface().api()) {
      cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;

      // args - definition;
      int arg_count = 0;
      for (auto const& arg : api.arg()) {
        if (arg.is_callback()) {  // arg.type() isn't always TYPE_FUNCTION_POINTER
          if (arg.type() == TYPE_PREDEFINED) {
            string name = "vts_callback_" + fuzzer_extended_class_name + "_" +
                arg.predefined_type();  // TODO - check to make sure name
                                        // is always correct
            if (name.back() == '*') name.pop_back();
            cpp_ss << "    " << name << "* arg" << arg_count << "callback = new ";
            cpp_ss << name << "(callback_socket_name);" << endl;
            cpp_ss << "    arg" << arg_count << "callback->Register(func_msg->arg("
                   << arg_count << "));" << endl;

            cpp_ss << "    " << GetCppVariableType(arg, &message) << " ";
            cpp_ss << "arg" << arg_count << " = (" << GetCppVariableType(arg, &message)
                   << ") malloc(sizeof(" << GetCppVariableType(arg, &message) << "));"
                   << endl;
            // TODO: think about how to free the malloced callback data structure.
            // find the spec.
            bool found = false;
            cout << name << endl;
            for (int attr_idx = 0;
                 attr_idx < message.attribute_size() + message.interface().attribute_size();
                 attr_idx++) {
              const VariableSpecificationMessage& attribute = (attr_idx < message.attribute_size()) ?
                  message.attribute(attr_idx) :
                  message.interface().attribute(attr_idx - message.attribute_size());

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
          } else if (arg.type() == TYPE_HIDL_CALLBACK) {
            cpp_ss << "    sp<" << arg.predefined_type()  << "> arg" << arg_count
                   << "(" << "VtsFuzzerCreate"
                   << arg.predefined_type() << "(callback_socket_name));" << endl;
          }
        } else {
          if (arg.type() == TYPE_VECTOR) {
            if (arg.vector_value(0).type() == TYPE_SCALAR) {
              cpp_ss << "    " << arg.vector_value(0).scalar_type() << "* "
                     << "arg" << arg_count << "buffer = ("
                     << arg.vector_value(0).scalar_type()
                     << "*) malloc("
                     << "func_msg->arg(" << arg_count
                     << ").vector_size()"
                     << " * sizeof("
                     << arg.vector_value(0).scalar_type() << "));"
                     << endl;
            } else if (arg.vector_value(0).type() == TYPE_STRUCT) {
              cpp_ss << "    " << arg.vector_value(0).struct_type() << "* "
                     << "arg" << arg_count << "buffer = ("
                     << arg.vector_value(0).struct_type()
                     << "*) malloc("
                     << "func_msg->arg(" << arg_count
                     << ").vector_size()"
                     << " * sizeof("
                     << arg.vector_value(0).struct_type() << "));"
                     << endl;
            } else {
              cerr << __func__ << " " << __LINE__ << " ERROR unsupported vector sub-type "
                   << arg.vector_value(0).type() << endl;
            }
          }

          if (arg.type() == TYPE_HIDL_CALLBACK) {
            cpp_ss << "    sp<" << arg.predefined_type()  << "> arg" << arg_count
                   << "(" << "VtsFuzzerCreate"
                   << arg.predefined_type() << "(callback_socket_name));" << endl;
          } else if (arg.type() == TYPE_STRUCT) {
            cpp_ss << "    " << GetCppVariableType(arg, &message) << " ";
            cpp_ss << "arg" << arg_count;
          } else if (arg.type() == TYPE_VECTOR) {
            cpp_ss << "    " << GetCppVariableType(arg, &message) << " ";
            cpp_ss << "arg" << arg_count << ";" << endl;
          } else {
            cpp_ss << "    " << GetCppVariableType(arg, &message) << " ";
            cpp_ss << "arg" << arg_count << " = ";
          }

          if (arg.type() != TYPE_VECTOR &&
              arg.type() != TYPE_HIDL_CALLBACK &&
              arg.type() != TYPE_STRUCT) {
            std::stringstream msg_ss;
            msg_ss << "func_msg->arg(" << arg_count << ")";
            string msg = msg_ss.str();

            if (arg.type() == TYPE_SCALAR) {
              cpp_ss << "(" << msg << ".type() == TYPE_SCALAR)? ";
              if (arg.scalar_type() == "pointer" ||
                  arg.scalar_type() == "pointer_pointer" ||
                  arg.scalar_type() == "char_pointer" ||
                  arg.scalar_type() == "void_pointer" ||
                  arg.scalar_type() == "function_pointer") {
                cpp_ss << "reinterpret_cast<"
                       << GetCppVariableType(arg, &message)
                       << ">";
              }
              cpp_ss << "(" << msg << ".scalar_value()";

              if (arg.scalar_type() == "int32_t" ||
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
                         arg.scalar_type() == "void_pointer") {
                cpp_ss << ".pointer() ";
              } else {
                cerr << __func__ << " ERROR unsupported scalar type "
                     << arg.scalar_type() << endl;
                exit(-1);
              }
              cpp_ss << ") : ";
            } else if (arg.type() == TYPE_ENUM) {
              // TODO: impl
            } else {
              cerr << __func__ << ":" << __LINE__ << " unknown type "
                   << arg.type() << endl;
              exit(-1);
            }

            cpp_ss << "( (" << msg << ".type() == TYPE_PREDEFINED || " << msg
                   << ".type() == TYPE_STRUCT || " << msg
                   << ".type() == TYPE_SCALAR)? ";
            cpp_ss << GetCppInstanceType(arg, msg, &message);
            cpp_ss << " : " << GetCppInstanceType(arg, string(), &message) << " )";
            // TODO: use the given message and call a lib function which converts
            // a message to a C/C++ struct.
          } else if (arg.type() == TYPE_VECTOR) {
            // TODO: dynamically generate the initial value for hidl_vec
            cpp_ss << "    for (int vector_index = 0; vector_index < "
                   << "func_msg->arg(" << arg_count << ").vector_size(); "
                   << "vector_index++) {" << endl;
            cpp_ss << "      arg" << arg_count << "buffer[vector_index] = "
                   << "func_msg->arg(" << arg_count << ").vector_value(vector_index)."
                   << "scalar_value()." << arg.vector_value(0).scalar_type() << "();"
                   << endl;
            cpp_ss << "    }" << endl;
            cpp_ss << "arg" << arg_count << ".setToExternal("
                   << "arg" << arg_count << "buffer, "
                   << "func_msg->arg(" << arg_count << ").vector_size()"
                   << ")";
          }
          cpp_ss << ";" << endl;
          if (arg.type() == TYPE_STRUCT) {
            if (message.component_class() == HAL_HIDL) {
              cpp_ss << "    MessageTo" << GetCppVariableType(arg, &message)
                     << "(func_msg->arg(" << arg_count << "), &arg" << arg_count << ");" << endl;
            }
          }
        }
        arg_count++;
      }

      // actual function call
      GenerateCodeToStartMeasurement(cpp_ss);

      // may need to check whether the function is actually defined.
      cpp_ss << "    cout << \"Call an API\" << endl;" << endl;
      cpp_ss << "    cout << \"local_device = \" << " << kInstanceVariableName
             << ".get();" << endl;
      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        cpp_ss << "    *result = NULL;" << endl;
        cpp_ss << "    " << kInstanceVariableName << "->" << api.name() << "(";
      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR ||
                 api.return_type_hidl(0).type() == TYPE_ENUM) {
        cpp_ss << "    *result = reinterpret_cast<void*>("
               << "(" << api.return_type_hidl(0).scalar_type() << ")"
               << kInstanceVariableName << "->" << api.name() << "(";
      } else {
        cpp_ss << "    *result = const_cast<void*>(reinterpret_cast<"
               << "const void*>(new string("
               << kInstanceVariableName << "->" << api.name() << "(";
      }
      if (arg_count > 0) cpp_ss << endl;

      for (int index = 0; index < arg_count; index++) {
        cpp_ss << "      arg" << index;
        if (index != (arg_count - 1)) {
          cpp_ss << "," << endl;
        }
      }

      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        cpp_ss << ");" << endl;
      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR ||
                 api.return_type_hidl(0).type() == TYPE_ENUM) {
        cpp_ss << "));"
               << endl;
      } else {
        if (api.return_type_hidl_size() > 0) {
          if (arg_count != 0) cpp_ss << ", ";
          cpp_ss << fuzzer_extended_class_name << api.name() << "_cb";
        }
        cpp_ss << ").toString8().string())));" << endl;
      }

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
  } else if (message.component_name() == "types") {
    for (int attr_idx = 0;
         attr_idx < message.attribute_size() + message.interface().attribute_size();
         attr_idx++) {
      const VariableSpecificationMessage& attribute = (attr_idx < message.attribute_size()) ?
          message.attribute(attr_idx) :
          message.interface().attribute(attr_idx - message.attribute_size());

      if (attribute.type() == TYPE_ENUM) {
        cpp_ss << attribute.name() << " " << "Random" << attribute.name() << "() {"
               << endl;
        cpp_ss << "int choice = rand() / " << attribute.enum_value().enumerator().size() << ";" << endl;
        cpp_ss << "if (choice < 0) choice *= -1;" << endl;
        for (int index = 0; index < attribute.enum_value().enumerator().size(); index++) {
          cpp_ss << "    if (choice == ";
          if (attribute.enum_value().scalar_type() == "int32_t") {
            cpp_ss << attribute.enum_value().scalar_value(index).int32_t();
          } else if (attribute.enum_value().scalar_type() == "uint32_t") {
            cpp_ss << attribute.enum_value().scalar_value(index).uint32_t();
          } else {
            cerr << __func__ << ":" << __LINE__ << " ERROR unsupported enum type "
                 << attribute.enum_value().scalar_type() << endl;
            exit(-1);
          }
          cpp_ss << ") return " << attribute.name() << "::"
                 << attribute.enum_value().enumerator(index)
                 << ";" << endl;
        }
        cpp_ss << "    return " << attribute.name() << "::"
               << attribute.enum_value().enumerator(0)
               << ";" << endl;
        cpp_ss << "}" << endl;
      } else if (attribute.type() == TYPE_STRUCT) {
        cpp_ss << "void " << "MessageTo" << attribute.name()
               << "(const VariableSpecificationMessage& var_msg, "
               << attribute.name() << "* arg) {"
               << endl;
        int struct_index = 0;
        for (const auto& struct_value : attribute.struct_value()) {
          if (struct_value.type() == TYPE_SCALAR) {
            cpp_ss << "    arg->" << struct_value.name() << " = "
                   << "var_msg.struct_value(" << struct_index
                   << ").scalar_value()." << struct_value.scalar_type() << "();"
                   << endl;
          } else if (struct_value.type() == TYPE_STRING) {  // convert to hidl_string
            cpp_ss << "    arg->" << struct_value.name() << ".setToExternal("
                   << "\"random\", 7);"
                   << endl;
          } else if (struct_value.type() == TYPE_VECTOR) {
            cpp_ss << "  arg->" << struct_value.name() << ".resize("
                   << "var_msg.struct_value(" << struct_index
                   << ").vector_size()"
                   << ");" << endl;
            if (struct_value.vector_value(0).type() == TYPE_SCALAR) {
              cpp_ss << "  for (int value_index = 0; value_index < "
                     << "var_msg.struct_value(" << struct_index
                     << ").vector_size(); "
                     << "value_index++) {" << endl;
              cpp_ss << "    arg->" << struct_value.name() << "[value_index] = "
                     << "var_msg.struct_value(" << struct_index
                     << ").vector_value(value_index).scalar_value()."
                     << struct_value.vector_value(0).scalar_type() << "();" << endl;
              cpp_ss << "  }" << endl;
            } else if (struct_value.vector_value(0).type() == TYPE_STRUCT) {
              // Should use a recursion to handle nested data structure.
              cpp_ss << "  for (int value_index = 0; value_index < "
                     << "var_msg.struct_value(" << struct_index
                     << ").vector_size(); "
                     << "value_index++) {" << endl;
              int sub_value_index = 0;
              for (const auto& sub_struct_value : struct_value.vector_value(0).struct_value()) {
                if (sub_struct_value.type() == TYPE_SCALAR) {
                  cpp_ss << "    arg->" << struct_value.name() << "[value_index]."
                         << sub_struct_value.name() << " = "
                         << "var_msg.struct_value(" << struct_index
                         << ").vector_value(value_index).struct_value("
                         << sub_value_index << ").scalar_type()."
                         << sub_struct_value.scalar_type() << "();" << endl;
                  sub_value_index++;
                } else {
                  cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                       << sub_struct_value.type() << endl;
                  exit(-1);
                }
              }
              cpp_ss << "  }" << endl;
            } else {
              cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                   << struct_value.vector_value(0).type() << endl;
              exit(-1);
            }
          } else {
            cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                 << struct_value.type() << endl;
            exit(-1);
          }
          struct_index++;
        }
        cpp_ss << "}" << endl;
      } else {
        cerr << __func__ << " unsupported attribute type "
             << attribute.type() << endl;
      }
    }
  }
}

void HalHidlCodeGen::GenerateCppBodyFuzzFunction(
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

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      cpp_ss << "    " << GetCppVariableType(arg) << " ";
      cpp_ss << "arg" << arg_count << " = ";
      {
        std::stringstream msg_ss;
        msg_ss << "func_msg->arg(" << arg_count << ")";
        string msg = msg_ss.str();

        if (arg.type() == TYPE_SCALAR) {
          cpp_ss << "(" << msg << ".type() == TYPE_SCALAR && " << msg
                 << ".scalar_value()";
          if (arg.scalar_type() == "pointer" ||
              arg.scalar_type() == "char_pointer" ||
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
              arg.scalar_type() == "int8_t" ||
              arg.scalar_type() == "uint8_t" ||
              arg.scalar_type() == "float_t" ||
              arg.scalar_type() == "double_t") {
            cpp_ss << "." << arg.scalar_type() << "() ";
          } else if (arg.scalar_type() == "pointer" ||
                     arg.scalar_type() == "char_pointer" ||
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

        cpp_ss << "( (" << msg << ".type() == TYPE_PREDEFINED || " << msg
               << ".type() == TYPE_STRUCT || " << msg
               << ".type() == TYPE_SCALAR)? ";
        cpp_ss << GetCppInstanceType(arg, msg);
        cpp_ss << " : " << GetCppInstanceType(arg, string()) << " )";
        // TODO: use the given message and call a lib function which converts
        // a message to a C/C++ struct.
      }
      cpp_ss << ";" << endl;
      arg_count++;
    }

    // actual function call
    GenerateCodeToStartMeasurement(cpp_ss);

    cpp_ss << "    cout << \"Call an API.\" << endl;" << endl;
    cpp_ss << "    cout << \"local_device = \" << " << kInstanceVariableName
           << ".get();" << endl;
    cpp_ss << "    *result = const_cast<void*>(reinterpret_cast<const void*>(new string("
           << kInstanceVariableName << "->" << parent_path << message.name()
           << "->" << api.name() << "(";
    if (arg_count > 0) cpp_ss << endl;

    for (int index = 0; index < arg_count; index++) {
      cpp_ss << "      arg" << index;
      if (index != (arg_count - 1)) {
        cpp_ss << "," << endl;
      }
    }
    if (api.return_type_hidl_size() > 0) {
      if (arg_count != 0) cpp_ss << ", ";
      cpp_ss << fuzzer_extended_class_name << api.name() << "_cb";
      // TODO: support callback as the last arg and setup barrier here to
      // do *result = ...;
    }
    cpp_ss << ").toString8().string())));" << endl;
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

void HalHidlCodeGen::GenerateCppBodyGetAttributeFunction(
    std::stringstream& cpp_ss, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  if (message.component_name() != "types" &&
      !endsWith(message.component_name(), "Callback")) {
    cpp_ss << "bool " << fuzzer_extended_class_name << "::GetAttribute(" << endl;
    cpp_ss << "    FunctionSpecificationMessage* func_msg," << endl;
    cpp_ss << "    void** result) {" << endl;

    // TOOD: impl
    cerr << __func__ << " not supported for HIDL HAL yet" << endl;

    cpp_ss << "  cerr << \"attribute not found\" << endl;" << endl;
    cpp_ss << "  return false;" << endl;
    cpp_ss << "}" << endl;
  }
}

void HalHidlCodeGen::GenerateHeaderGlobalFunctionDeclarations(
    std::stringstream& h_ss, const string& function_prototype) {
  h_ss << "extern \"C\" {" << endl;
  h_ss << "extern " << function_prototype << ";" << endl;
  h_ss << "}" << endl;
}

void HalHidlCodeGen::GenerateCppBodyGlobalFunctions(
    std::stringstream& cpp_ss, const string& function_prototype,
    const string& fuzzer_extended_class_name) {
  cpp_ss << "extern \"C\" {" << endl;
  cpp_ss << function_prototype << " {" << endl;
  cpp_ss << "  return (android::vts::FuzzerBase*) "
         << "new android::vts::" << fuzzer_extended_class_name << "();" << endl;
  cpp_ss << "}" << endl << endl;
  cpp_ss << "}" << endl;
}

void HalHidlCodeGen::GenerateSubStructFuzzFunctionCall(
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
