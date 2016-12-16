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
#include "utils/StringUtil.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const HalHidlCodeGen::kInstanceVariableName = "hw_binder_proxy_";

void HalHidlCodeGen::GenerateCppBodyCallbackFunction(
    Formatter& out, const ComponentSpecificationMessage& message,
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
      out << "static string callback_socket_name_;" << "\n";
      first_callback = false;
    }
    out << "\n";
    out << "class " << name << " : public FuzzerCallbackBase {" << "\n";
    out << " public:" << "\n";
    out.indent();
    out << name << "(const string& callback_socket_name) {" << "\n";
    out << "    callback_socket_name_ = callback_socket_name;" << "\n";
    out << "  }" << "\n";

    int primitive_format_index = 0;
    for (const FunctionPointerSpecificationMessage& func_pt_spec :
         attribute.function_pointer()) {
      const string& callback_name = func_pt_spec.function_name();
      // TODO: callback's return value is assumed to be 'void'.
      out << "\n";
      out << "static ";
      bool has_return_value = false;
      if (!func_pt_spec.has_return_type() ||
          !func_pt_spec.return_type().has_type() ||
          func_pt_spec.return_type().type() == TYPE_VOID) {
        out << "void" << "\n";
      } else if (func_pt_spec.return_type().type() == TYPE_PREDEFINED) {
        out << func_pt_spec.return_type().predefined_type();
        has_return_value = true;
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR unknown type "
             << func_pt_spec.return_type().type() << "\n";
        exit(-1);
      }
      out << " " << callback_name << "(";
      int primitive_type_index;
      primitive_type_index = 0;
      for (const auto& arg : func_pt_spec.arg()) {
        if (primitive_type_index != 0) {
          out << ", ";
        }
        if (arg.is_const()) {
          out << "const ";
        }
        if (arg.type() == TYPE_SCALAR) {
          GenerateScalarTypeInC(out, arg.scalar_type());
          out << " ";
        } else if (arg.type() == TYPE_PREDEFINED) {
          out << arg.predefined_type() << " ";
        } else {
          cerr << __func__ << " unsupported type" << "\n";
          exit(-1);
        }
        out << "arg" << primitive_type_index;
        primitive_type_index++;
      }
      out << ") {" << "\n";
      out.indent();
#if USE_VAARGS
      out << "const char fmt[] = \""
             << definition.primitive_format(primitive_format_index) << "\";"
             << "\n";
      out << "va_list argp;" << "\n";
      out << "const char* p;" << "\n";
      out << "int i;" << "\n";
      out << "char* s;" << "\n";
      out << "char fmtbuf[256];" << "\n";
      out << "\n";
      out << "va_start(argp, fmt);" << "\n";
      out << "\n";
      out << "for (p = fmt; *p != '\\0'; p++) {" << "\n";
      out.indent();
      out << "if (*p != '%') {" << "\n";
      out.indent();
      out << "putchar(*p);" << "\n";
      out << "continue;" << "\n";
      out.unindent();
      out << "}" << "\n";
      out.unindent();
      out << "switch (*++p) {" << "\n";
      out.indent();
      out << "case 'c':" << "\n";
      out.indent();
      out << "i = va_arg(argp, int);" << "\n";
      out << "putchar(i);" << "\n";
      out << "break;" << "\n";
      out.unindent();
      out << "case 'd':" << "\n";
      out.indent();
      out << "i = va_arg(argp, int);" << "\n";
      out << "s = itoa(i, fmtbuf, 10);" << "\n";
      out << "fputs(s, stdout);" << "\n";
      out << "break;" << "\n";
      out.unindent();
      out << "case 's':" << "\n";
      out.indent();
      out << "s = va_arg(argp, char *);" << "\n";
      out << "fputs(s, stdout);" << "\n";
      out << "break;" << "\n";
      out.unindent();
      // out << "        case 'p':
      out << "case '%':" << "\n";
      out.indent();
      out << "putchar('%');" << "\n";
      out << "break;" << "\n";
      out.unindent();
      out << "}" << "\n";
      out.unindent();
      out << "}" << "\n";
      out << "va_end(argp);" << "\n";
#endif
      // TODO: check whether bytes is set and handle properly if not.
      out << "AndroidSystemCallbackRequestMessage callback_message;"
             << "\n";
      out << "callback_message.set_id(GetCallbackID(\"" << callback_name
             << "\"));" << "\n";
      out << "RpcCallToAgent(callback_message, callback_socket_name_);"
             << "\n";
      if (has_return_value) {
        // TODO: consider actual return type.
        out << "return NULL;";
      }
      out.unindent();
      out << "}" << "\n";
      out << "\n";

      primitive_format_index++;
    }
    out << "\n";
    out.unindent();
    out << " private:" << "\n";
    out << "};" << "\n";
    out << "\n";
  }
}


void HalHidlCodeGen::GenerateCppBodySyncCallbackFunction(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {

  for (auto const& api : message.interface().api()) {
    if (api.return_type_hidl_size() > 0) {
      out << "static void " << fuzzer_extended_class_name
          << api.name() << "_cb_func(";
      bool first_return_type = true;
      int arg_index = 0;
      for (const auto& return_type_hidl : api.return_type_hidl()) {
        if (first_return_type) {
          first_return_type = false;
        } else {
          out << ", ";
        }
        if (return_type_hidl.type() == TYPE_SCALAR) {
          GenerateScalarTypeInC(out, return_type_hidl.scalar_type());
        } else if (return_type_hidl.type() == TYPE_ENUM ||
                   return_type_hidl.type() == TYPE_VECTOR) {
          out << GetCppVariableType(return_type_hidl, &message);
        } else if (return_type_hidl.type() == TYPE_STRING) {
          out << "::android::hardware::hidl_string ";
        } else if (return_type_hidl.type() == TYPE_STRUCT) {
          if (return_type_hidl.has_predefined_type()) {
            out << return_type_hidl.predefined_type() << " ";
          } else {
            cerr << __func__ << ":" << __LINE__ << " ERROR no predefined type "
                 << "\n";
            exit(-1);
          }
        } else {
          cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
               << return_type_hidl.type() << " for " << api.name() << "\n";
          exit(-1);
        }
        out << " arg" << arg_index;
        arg_index++;
      }
      out << ") {" << "\n";
      // TODO: support other non-scalar type and multiple args.
      out << "  cout << \"callback " << api.name() << " called\""
             << " << endl;" << "\n";
      out << "}" << "\n";
      out << "std::function<" << "void(";
      first_return_type = true;
      for (const auto& return_type_hidl : api.return_type_hidl()) {
        if (first_return_type) {
          first_return_type = false;
        } else {
          out << ", ";
        }
        if (return_type_hidl.type() == TYPE_SCALAR) {
          GenerateScalarTypeInC(out, return_type_hidl.scalar_type());
        } else if (return_type_hidl.type() == TYPE_ENUM ||
                   return_type_hidl.type() == TYPE_VECTOR ||
                   return_type_hidl.type() == TYPE_STRUCT) {
          out << GetCppVariableType(return_type_hidl, &message);
        } else if (return_type_hidl.type() == TYPE_STRING) {
          out << "::android::hardware::hidl_string";
        } else {
          cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
               << return_type_hidl.type() << " for " << api.name() << "\n";
          exit(-1);
        }
      }
      out << ")> "
             << fuzzer_extended_class_name << api.name() << "_cb = "
             << fuzzer_extended_class_name << api.name() << "_cb_func;" << "\n";
      out << "\n" << "\n";
    }
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
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  GenerateCppBodySyncCallbackFunction(
      out, message, fuzzer_extended_class_name);

  set<string> callbacks;
  for (auto const& api : message.interface().api()) {
    for (auto const& arg : api.arg()) {
      if (!arg.is_callback() && arg.type() == TYPE_HIDL_CALLBACK &&
          callbacks.find(arg.predefined_type()) == callbacks.end()) {
        out << "extern " << arg.predefined_type() << "* "
               << "VtsFuzzerCreate" << arg.predefined_type()
               << "(const string& callback_socket_name);" << "\n" << "\n";
        callbacks.insert(arg.predefined_type());
      }
    }
  }

  if (message.component_name() != "types" &&
      !endsWith(message.component_name(), "Callback")) {
    for (auto const& sub_struct : message.interface().sub_struct()) {
      GenerateCppBodyFuzzFunction(out, sub_struct, fuzzer_extended_class_name,
                                  message.original_data_structure_name(),
                                  sub_struct.is_pointer() ? "->" : ".");
    }

    out << "bool " << fuzzer_extended_class_name
        << "::GetService(bool get_stub) {" << "\n";
    out.indent();

    out << "static bool initialized = false;" << "\n";
    out << "if (!initialized) {" << "\n";
    out.indent();
    out << "cout << \"[agent:hal] HIDL getService\" << endl;" << "\n";
    string service_name = message.package().substr(
        message.package().find_last_of(".") + 1);
    if (service_name == "nfc") {
      // TODO(yim): remove this special case after b/32158398 is fixed.
      service_name = "nfc_nci";
    }
    out << "hw_binder_proxy_ = " << message.component_name()
        << "::getService(\""
        << service_name
        << "\", get_stub);" << "\n";
    out << "cout << \"[agent:hal] hw_binder_proxy_ = \" << "
        << "hw_binder_proxy_.get() << endl;" << "\n";
    out << "initialized = true;" << "\n";
    out.unindent();
    out << "}" << "\n";
    out << "return true;" << "\n";
    out.unindent();
    out << "}" << "\n" << "\n";

    out << "bool " << fuzzer_extended_class_name << "::Fuzz(" << "\n";
    out << "    FunctionSpecificationMessage* func_msg," << "\n";
    out << "    void** result, const string& callback_socket_name) {" << "\n";
    out.indent();

    out << "const char* func_name = func_msg->name().c_str();" << "\n";
    out << "cout << \"Function: \" << __func__ << \" \" << func_name << endl;"
        << "\n";

    // to call another function if it's for a sub_struct
    if (message.interface().sub_struct().size() > 0) {
      out << "if (func_msg->parent_path().length() > 0) {" << "\n";
      out.indent();
      for (auto const& sub_struct : message.interface().sub_struct()) {
        GenerateSubStructFuzzFunctionCall(out, sub_struct, "");
      }
      out.unindent();
      out << "}" << "\n";
    }

    for (auto const& api : message.interface().api()) {
      out << "if (!strcmp(func_name, \"" << api.name() << "\")) {" << "\n";
      out.indent();

      // args - definition;
      int arg_count = 0;
      for (auto const& arg : api.arg()) {
        if (arg.is_callback()) {  // arg.type() isn't always TYPE_FUNCTION_POINTER
          if (arg.type() == TYPE_PREDEFINED) {
            string name = "vts_callback_" + fuzzer_extended_class_name + "_" +
                arg.predefined_type();  // TODO - check to make sure name
                                        // is always correct
            if (name.back() == '*') name.pop_back();
            out << name << "* arg" << arg_count << "callback = new ";
            out << name << "(callback_socket_name);" << "\n";
            out << "arg" << arg_count << "callback->Register(func_msg->arg("
                << arg_count << "));" << "\n";

            out << GetCppVariableType(arg, &message) << " ";
            out << "arg" << arg_count << " = (" << GetCppVariableType(arg, &message)
                << ") malloc(sizeof(" << GetCppVariableType(arg, &message) << "));"
                << "\n";
            // TODO: think about how to free the malloced callback data structure.
            // find the spec.
            bool found = false;
            cout << name << "\n";
            for (int attr_idx = 0;
                 attr_idx < message.attribute_size() + message.interface().attribute_size();
                 attr_idx++) {
              const VariableSpecificationMessage& attribute = (attr_idx < message.attribute_size()) ?
                  message.attribute(attr_idx) :
                  message.interface().attribute(attr_idx - message.attribute_size());

              if (attribute.type() == TYPE_FUNCTION_POINTER &&
                  attribute.is_callback()) {
                string target_name = "vts_callback_" +
                    fuzzer_extended_class_name + "_" + attribute.name();
                cout << "compare" << "\n";
                cout << target_name << "\n";
                if (name == target_name) {
                  if (attribute.function_pointer_size() > 1) {
                    for (auto const& func_pt : attribute.function_pointer()) {
                      out << "arg" << arg_count << "->"
                          << func_pt.function_name() << " = arg" << arg_count
                          << "callback->" << func_pt.function_name() << ";"
                          << "\n";
                    }
                  } else {
                    out << "arg" << arg_count << " = arg" << arg_count
                        << "callback->" << attribute.name() << ";" << "\n";
                  }
                  found = true;
                  break;
                }
              }
            }
            if (!found) {
              cerr << __func__ << " ERROR callback definition missing for " << name
                   << " of " << api.name() << "\n";
              exit(-1);
            }
          } else if (arg.type() == TYPE_HIDL_CALLBACK) {
            out << "sp<" << arg.predefined_type()  << "> arg" << arg_count
                << "(" << "VtsFuzzerCreate"
                << arg.predefined_type() << "(callback_socket_name));" << "\n";
          }
        } else {
          if (arg.type() == TYPE_VECTOR) {
            if (arg.vector_value(0).type() == TYPE_SCALAR) {
              out << arg.vector_value(0).scalar_type() << "* "
                  << "arg" << arg_count << "buffer = ("
                  << arg.vector_value(0).scalar_type()
                  << "*) malloc("
                  << "func_msg->arg(" << arg_count
                  << ").vector_size()"
                  << " * sizeof("
                  << arg.vector_value(0).scalar_type() << "));"
                  << "\n";
            } else if (arg.vector_value(0).type() == TYPE_STRUCT) {
              out << arg.vector_value(0).predefined_type() << "* "
                  << "arg" << arg_count << "buffer = ("
                  << arg.vector_value(0).predefined_type()
                  << "*) malloc("
                  << "func_msg->arg(" << arg_count
                  << ").vector_size()"
                  << " * sizeof("
                  << arg.vector_value(0).predefined_type() << "));"
                  << "\n";
            } else if (arg.vector_value(0).type() == TYPE_ENUM) {
              out << arg.vector_value(0).predefined_type() << "* "
                  << "arg" << arg_count << "buffer = ("
                  << arg.vector_value(0).predefined_type()
                  << "*) malloc("
                  << "func_msg->arg(" << arg_count
                  << ").vector_size()"
                  << " * sizeof("
                  << arg.vector_value(0).predefined_type() << "));"
                  << "\n";
            } else {
              cerr << __func__ << ":" << __LINE__ << " ERROR unsupported vector sub-type "
                   << arg.vector_value(0).type() << "\n";
            }
          }

          if (arg.type() == TYPE_HIDL_CALLBACK) {
            out << "sp<" << arg.predefined_type()  << "> arg" << arg_count
                << "(" << "VtsFuzzerCreate"
                << arg.predefined_type() << "(callback_socket_name));" << "\n";
          } else if (arg.type() == TYPE_STRUCT) {
            out << GetCppVariableType(arg, &message) << " ";
            out << "arg" << arg_count;
          } else if (arg.type() == TYPE_VECTOR) {
            out << GetCppVariableType(arg, &message) << " ";
            out << "arg" << arg_count << ";" << "\n";
          } else {
            out << GetCppVariableType(arg, &message) << " ";
            out << "arg" << arg_count << " = ";
          }

          if (arg.type() != TYPE_VECTOR &&
              arg.type() != TYPE_HIDL_CALLBACK &&
              arg.type() != TYPE_STRUCT) {
            std::stringstream msg_ss;
            msg_ss << "func_msg->arg(" << arg_count << ")";
            string msg = msg_ss.str();

            if (arg.type() == TYPE_SCALAR) {
              out << "(" << msg << ".type() == TYPE_SCALAR)? ";
              if (arg.scalar_type() == "pointer" ||
                  arg.scalar_type() == "pointer_pointer" ||
                  arg.scalar_type() == "char_pointer" ||
                  arg.scalar_type() == "void_pointer" ||
                  arg.scalar_type() == "function_pointer") {
                out << "reinterpret_cast<"
                    << GetCppVariableType(arg, &message)
                    << ">";
              }
              out << "(" << msg << ".scalar_value()";

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
                out << "." << arg.scalar_type() << "() ";
              } else if (arg.scalar_type() == "pointer" ||
                         arg.scalar_type() == "char_pointer" ||
                         arg.scalar_type() == "void_pointer") {
                out << ".pointer() ";
              } else {
                cerr << __func__ << " ERROR unsupported scalar type "
                     << arg.scalar_type() << "\n";
                exit(-1);
              }
              out << ") : ";
            } else if (arg.type() == TYPE_ENUM) {
              // TODO(yim): support this case
              out << "/* enum support */" << "\n";
            } else if (arg.type() == TYPE_STRING) {
              // TODO(yim): support this case
              cerr << __func__ << ":" << __LINE__ << " unknown type "
                   << arg.type() << "\n";
            } else {
              cerr << __func__ << ":" << __LINE__ << " unknown type "
                   << arg.type() << "\n";
              exit(-1);
            }

            out << "( (" << msg << ".type() == TYPE_PREDEFINED || " << msg
                << ".type() == TYPE_STRUCT || " << msg
                << ".type() == TYPE_SCALAR)? ";
            out << GetCppInstanceType(arg, msg, &message);
            out << " : " << GetCppInstanceType(arg, string(), &message) << " )";
            // TODO: use the given message and call a lib function which converts
            // a message to a C/C++ struct.
          } else if (arg.type() == TYPE_VECTOR) {
            // TODO: dynamically generate the initial value for hidl_vec
            out << "for (int vector_index = 0; vector_index < "
                << "func_msg->arg(" << arg_count << ").vector_size(); "
                << "vector_index++) {" << "\n";
            out.indent();
            if (arg.vector_value(0).type() == TYPE_SCALAR) {
              out << "arg" << arg_count << "buffer[vector_index] = "
                  << "func_msg->arg(" << arg_count << ").vector_value(vector_index)."
                  << "scalar_value()." << arg.vector_value(0).scalar_type() << "();"
                  << "\n";
            } else if (arg.vector_value(0).type() == TYPE_ENUM) {
              std::string enum_attribute_name = arg.vector_value(0).predefined_type();
              ReplaceSubString(enum_attribute_name, "::", "__");
              out << "arg" << arg_count << "buffer[vector_index] = "
                  << "EnumValue" << enum_attribute_name
                  << "(func_msg->arg(" << arg_count << ").vector_value(vector_index)."
                  << "enum_value());"
                  << "\n";
            } else if (arg.vector_value(0).type() == TYPE_STRUCT) {
              out << "/* arg" << arg_count << "buffer[vector_index] not initialized "
                  << "since TYPE_STRUCT not yet supported */" << "\n";
            } else {
              cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                   << arg.vector_value(0).type() << "\n";
              exit(-1);
            }
            out.unindent();
            out << "}" << "\n";
            out << "arg" << arg_count << ".setToExternal("
                << "arg" << arg_count << "buffer, "
                << "func_msg->arg(" << arg_count << ").vector_size()"
                << ")";
          }
          out << ";" << "\n";
          if (arg.type() == TYPE_STRUCT) {
            if (message.component_class() == HAL_HIDL) {
              std::string attribute_name = arg.predefined_type();
              ReplaceSubString(attribute_name, "::", "__");
              out << "MessageTo" << attribute_name
                  << "(func_msg->arg(" << arg_count
                  << "), &arg" << arg_count << ");\n";
            }
          }
        }
        arg_count++;
      }

      // actual function call
      GenerateCodeToStartMeasurement(out);

      // may need to check whether the function is actually defined.
      out << "cout << \"Call an API\" << endl;" << "\n";
      out << "cout << \"local_device = \" << " << kInstanceVariableName
          << ".get();" << "\n";
      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl_size() > 1 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        // TODO(yim): support multiple return values (when size > 1)
        out << "*result = NULL;" << "\n";
        out << kInstanceVariableName << "->" << api.name() << "(";
      } else if (api.return_type_hidl_size() == 1 &&
                 api.return_type_hidl(0).type() != TYPE_SCALAR &&
                 api.return_type_hidl(0).type() != TYPE_ENUM) {
        out << "*result = NULL;" << "\n";
        out << kInstanceVariableName << "->" << api.name() << "(";
      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR) {
        out << "*result = reinterpret_cast<void*>("
            << "(";
        GenerateScalarTypeInC(out, api.return_type_hidl(0).scalar_type());
        out << ")" << kInstanceVariableName << "->" << api.name() << "(";
      } else if (api.return_type_hidl(0).type() == TYPE_ENUM) {
        if (api.return_type_hidl(0).has_scalar_type()) {
          out << "*result = reinterpret_cast<void*>("
              << "(" << api.return_type_hidl(0).scalar_type() << ")"
              << kInstanceVariableName << "->" << api.name() << "(";
        } else if (api.return_type_hidl(0).has_predefined_type()) {
          out << "    *result = reinterpret_cast<void*>("
              << "(";
          out << api.return_type_hidl(0).predefined_type() << ")"
              << kInstanceVariableName << "->" << api.name() << "(";
        } else {
          cerr << __func__ << ":" << __LINE__ << " unknown return type" << "\n";
          exit(-1);
        }
      } else {
        out << "*result = const_cast<void*>(reinterpret_cast<"
            << "const void*>(new string("
            << kInstanceVariableName << "->" << api.name() << "(";
      }
      if (arg_count > 0) out << "\n";

      out.indent();
      for (int index = 0; index < arg_count; index++) {
        out << "arg" << index;
        if (index != (arg_count - 1)) {
          out << "," << "\n";
        }
      }
      out.unindent();

      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        out << ");" << "\n";
      } else if (api.return_type_hidl_size() == 1) {
        if (api.return_type_hidl(0).type() == TYPE_SCALAR ||
            api.return_type_hidl(0).type() == TYPE_ENUM) {
          out << "));"
              << "\n";
        } else {
          if (arg_count != 0) out << ", ";
          out << fuzzer_extended_class_name << api.name() << "_cb_func";
          // TODO(yim): support non-scalar return type.
          out << ");" << "\n";
        }
      } else {
        if (arg_count != 0) out << ", ";
        out << fuzzer_extended_class_name << api.name() << "_cb_func";
        // return type is void
        // TODO(yim): support multiple return values
        out << ");" << "\n";
      }

      GenerateCodeToStopMeasurement(out);
      out << "cout << \"called\" << endl;" << "\n";

      // Copy the output (call by pointer or reference cases).
      arg_count = 0;
      for (auto const& arg : api.arg()) {
        if (arg.is_output()) {
          // TODO check the return value
          out << "    " << GetConversionToProtobufFunctionName(arg)
                 << "(arg" << arg_count << ", "
                 << "func_msg->mutable_arg(" << arg_count << "));" << "\n";
        }
        arg_count++;
      }

      out << "return true;" << "\n";
      out.unindent();
      out << "}" << "\n";
    }

    // TODO: if there were pointers, free them.
    out << "return false;" << "\n";
    out.unindent();
    out << "}" << "\n";
  } else if (message.component_name() == "types") {
    for (int attr_idx = 0;
         attr_idx < message.attribute_size() + message.interface().attribute_size();
         attr_idx++) {
      const VariableSpecificationMessage& attribute = (attr_idx < message.attribute_size()) ?
          message.attribute(attr_idx) :
          message.interface().attribute(attr_idx - message.attribute_size());

      if (attribute.type() == TYPE_ENUM) {
        std::string attribute_name = attribute.name();
        ReplaceSubString(attribute_name, "::", "__");

        // Message to value converter
        out << attribute.name() << " " << "EnumValue" << attribute_name
            << "(const EnumDataValueMessage& arg) {" << "\n";
        out.indent();
        out << "return (" << attribute.name()
            << ") arg.scalar_value(0)."
            << attribute.enum_value().scalar_type() << "();" << "\n";
        out.unindent();
        out << "}" << "\n";

        // Random value generator
        out << attribute.name() << " " << "Random" << attribute_name << "() {"
            << "\n";
        out.indent();
        out << attribute.enum_value().scalar_type() << " choice = "
            << "(" << attribute.enum_value().scalar_type() << ") "
            << "rand() / "
            << attribute.enum_value().enumerator().size() << ";" << "\n";
        if (attribute.enum_value().scalar_type().find("u") != 0) {
          out << "if (choice < 0) choice *= -1;" << "\n";
        }
        for (int index = 0; index < attribute.enum_value().enumerator().size(); index++) {
          out << "if (choice == ";
          out << "(" << attribute.enum_value().scalar_type() << ") ";
          if (attribute.enum_value().scalar_type() == "int32_t") {
            out << attribute.enum_value().scalar_value(index).int32_t();
          } else if (attribute.enum_value().scalar_type() == "uint32_t") {
            out << attribute.enum_value().scalar_value(index).uint32_t();
          } else {
            cerr << __func__ << ":" << __LINE__ << " ERROR unsupported enum type "
                 << attribute.enum_value().scalar_type() << "\n";
            exit(-1);
          }
          out << ") return " << attribute.name() << "::"
              << attribute.enum_value().enumerator(index)
              << ";" << "\n";
        }
        out << "return "
            << attribute.name() << "::"
            << attribute.enum_value().enumerator(0)
            << ";" << "\n";
        out.unindent();
        out << "}" << "\n";
      } else if (attribute.type() == TYPE_STRUCT) {
        std::string attribute_name = attribute.name();
        ReplaceSubString(attribute_name, "::", "__");
        out << "void " << "MessageTo" << attribute_name
            << "(const VariableSpecificationMessage& var_msg, "
            << attribute.name() << "* arg) {"
            << "\n";
        out.indent();
        int struct_index = 0;
        for (const auto& struct_value : attribute.struct_value()) {
          if (struct_value.type() == TYPE_SCALAR) {
            out << "arg->" << struct_value.name() << " = "
                << "var_msg.struct_value(" << struct_index
                << ").scalar_value()." << struct_value.scalar_type() << "();"
                << "\n";
          } else if (struct_value.type() == TYPE_STRING) {  // convert to hidl_string
            out << "arg->" << struct_value.name() << ".setToExternal("
                << "\"random\", 7);"
                << "\n";
          } else if (struct_value.type() == TYPE_VECTOR) {
            out << "arg->" << struct_value.name() << ".resize("
                << "var_msg.struct_value(" << struct_index
                << ").vector_size()"
                << ");" << "\n";
            if (struct_value.vector_value(0).type() == TYPE_SCALAR) {
              out << "for (int value_index = 0; value_index < "
                  << "var_msg.struct_value(" << struct_index
                  << ").vector_size(); "
                  << "value_index++) {" << "\n";
              out.indent();
              out << "arg->" << struct_value.name() << "[value_index] = "
                  << "var_msg.struct_value(" << struct_index
                  << ").vector_value(value_index).scalar_value()."
                  << struct_value.vector_value(0).scalar_type() << "();" << "\n";
              out.unindent();
              out << "}" << "\n";
            } else if (struct_value.vector_value(0).type() == TYPE_STRUCT) {
              // Should use a recursion to handle nested data structure.
              out << "for (int value_index = 0; value_index < "
                  << "var_msg.struct_value(" << struct_index
                  << ").vector_size(); "
                  << "value_index++) {" << "\n";
              out.indent();
              int sub_value_index = 0;
              for (const auto& sub_struct_value : struct_value.vector_value(0).struct_value()) {
                if (sub_struct_value.type() == TYPE_SCALAR) {
                  out << "    arg->" << struct_value.name() << "[value_index]."
                      << sub_struct_value.name() << " = "
                      << "var_msg.struct_value(" << struct_index
                      << ").vector_value(value_index).struct_value("
                      << sub_value_index << ").scalar_value()."
                      << sub_struct_value.scalar_type() << "();" << "\n";
                  sub_value_index++;
                } else if (sub_struct_value.type() == TYPE_ENUM) {
                  std::string enum_attribute_name = sub_struct_value.predefined_type();
                  ReplaceSubString(enum_attribute_name, "::", "__");
                  out << "    arg->" << struct_value.name() << "[value_index]."
                      << sub_struct_value.name() << " = "
                      << "EnumValue" << enum_attribute_name << "("
                      << "var_msg.struct_value(" << struct_index
                      << ").vector_value(value_index).struct_value("
                      << sub_value_index << ").enum_value());" << "\n";
                  sub_value_index++;
                } else if (sub_struct_value.type() == TYPE_STRING) {
                  out << "    arg->" << struct_value.name() << "[value_index]."
                      << sub_struct_value.name() << " = "
                      << "var_msg.struct_value(" << struct_index
                      << ").vector_value(value_index).struct_value("
                      << sub_value_index << ").string_value().message();"
                      << "\n";
                  sub_value_index++;
                } else {
                  cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                       << sub_struct_value.type() << "\n";
                  exit(-1);
                }
              }
              out.unindent();
              out << "}" << "\n";
            } else {
              cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                   << struct_value.vector_value(0).type() << "\n";
              exit(-1);
            }
          } else if (struct_value.type() == TYPE_ARRAY) {
            out << "for (int value_index = 0; value_index < "
                << "var_msg.struct_value(" << struct_index
                << ").vector_size(); "
                << "value_index++) {" << "\n";
            out.indent();
            if (struct_value.vector_value(0).type() == TYPE_SCALAR) {
              out << "arg->" << struct_value.name() << "[value_index] = "
                  << "var_msg.struct_value(" << struct_index
                  << ").vector_value(value_index).scalar_value()."
                  << struct_value.vector_value(0).scalar_type() << "();"
                  << "\n";
            } else {
              // TODO(yim): support other types and consider using a recursion or
              // at least common functions to generate such.
              cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                   << struct_value.vector_value(0).type() << "\n";
              exit(-1);
            }
            out.unindent();
            out << "}" << "\n";
          } else if (struct_value.type() == TYPE_ENUM) {
            std::string enum_attribute_name = struct_value.predefined_type();
            ReplaceSubString(enum_attribute_name, "::", "__");
            out << "arg->" << struct_value.name() << " = "
                << "EnumValue" << enum_attribute_name << "("
                << "var_msg.struct_value(" << struct_index
                << ").enum_value());" << "\n";
          } else if (struct_value.type() == TYPE_STRUCT) {
            if (struct_value.has_predefined_type()) {
              std::string struct_attribute_name = struct_value.predefined_type();
              ReplaceSubString(struct_attribute_name, "::", "__");
              out << "/*" << struct_value.predefined_type() << "*/";
              //"arg->" << struct_value.name() << " = "
              //    << "" << enum_attribute_name << "("
              //    << "var_msg.struct_value(" << struct_index
              //    << ").enum_value());" << "\n";
            } else {
              cerr << __func__ << ":" << __LINE__ << " ERROR predefined_type "
                   << "not defined." << "\n";
              exit(-1);
            }
          } else {
            cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                 << struct_value.type() << "\n";
            exit(-1);
          }
          struct_index++;
        }
        out.unindent();
        out << "}" << "\n";
      } else {
        cerr << __func__ << " unsupported attribute type "
             << attribute.type() << "\n";
      }
    }
  }
}

void HalHidlCodeGen::GenerateCppBodyFuzzFunction(
    Formatter& out, const StructSpecificationMessage& message,
    const string& fuzzer_extended_class_name,
    const string& original_data_structure_name, const string& parent_path) {
  for (auto const& sub_struct : message.sub_struct()) {
    GenerateCppBodyFuzzFunction(
        out, sub_struct, fuzzer_extended_class_name,
        original_data_structure_name,
        parent_path + message.name() + (sub_struct.is_pointer() ? "->" : "."));
  }

  string parent_path_printable(parent_path);
  ReplaceSubString(parent_path_printable, "->", "_");
  replace(parent_path_printable.begin(), parent_path_printable.end(), '.', '_');

  out << "bool " << fuzzer_extended_class_name << "::Fuzz_"
      << parent_path_printable + message.name() << "(" << "\n";
  out << "    FunctionSpecificationMessage* func_msg," << "\n";
  out << "    void** result, const string& callback_socket_name) {" << "\n";
  out << "  const char* func_name = func_msg->name().c_str();" << "\n";
  out << "  cout << \"Function: \" << __func__ << \" \" << func_name << endl;"
      << "\n";

  bool is_open;
  for (auto const& api : message.api()) {
    is_open = false;
    if ((parent_path_printable + message.name()) == "_common_methods" &&
        api.name() == "open") {
      is_open = true;
    }

    out << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << "\n";

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      out << "    " << GetCppVariableType(arg) << " ";
      out << "arg" << arg_count << " = ";
      {
        std::stringstream msg_ss;
        msg_ss << "func_msg->arg(" << arg_count << ")";
        string msg = msg_ss.str();

        if (arg.type() == TYPE_SCALAR) {
          out << "(" << msg << ".type() == TYPE_SCALAR && " << msg
              << ".scalar_value()";
          if (arg.scalar_type() == "pointer" ||
              arg.scalar_type() == "char_pointer" ||
              arg.scalar_type() == "void_pointer" ||
              arg.scalar_type() == "function_pointer") {
            out << ".has_pointer())? ";
            out << "reinterpret_cast<" << GetCppVariableType(arg) << ">";
          } else {
            out << ".has_" << arg.scalar_type() << "())? ";
          }
          out << "(" << msg << ".scalar_value()";

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
            out << "." << arg.scalar_type() << "() ";
          } else if (arg.scalar_type() == "pointer" ||
                     arg.scalar_type() == "char_pointer" ||
                     arg.scalar_type() == "function_pointer" ||
                     arg.scalar_type() == "void_pointer") {
            out << ".pointer() ";
          } else {
            cerr << __func__ << " ERROR unsupported type " << arg.scalar_type()
                 << "\n";
            exit(-1);
          }
          out << ") : ";
        }

        out << "((" << msg << ".type() == TYPE_PREDEFINED || " << msg
            << ".type() == TYPE_STRUCT || " << msg
            << ".type() == TYPE_SCALAR)? ";
        out << GetCppInstanceType(arg, msg);
        out << " : " << GetCppInstanceType(arg, string()) << " )";
        // TODO: use the given message and call a lib function which converts
        // a message to a C/C++ struct.
      }
      out << ";" << "\n";
      arg_count++;
    }

    // actual function call
    GenerateCodeToStartMeasurement(out);

    out.indent();
    out << "cout << \"Call an API.\" << endl;" << "\n";
    out << "cout << \"local_device = \" << " << kInstanceVariableName
        << ".get();" << "\n";
    out << "*result = const_cast<void*>(reinterpret_cast<const void*>(new string("
        << kInstanceVariableName << "->" << parent_path << message.name()
        << "->" << api.name() << "(";
    if (arg_count > 0) out << "\n";

    out.indent();
    for (int index = 0; index < arg_count; index++) {
      out << "arg" << index;
      if (index != (arg_count - 1)) {
        out << "," << "\n";
      }
    }
    out.unindent();

    if (api.return_type_hidl_size() > 0) {
      if (arg_count != 0) out << ", ";
      out << fuzzer_extended_class_name << api.name() << "_cb";
      // TODO: support callback as the last arg and setup barrier here to
      // do *result = ...;
    }
    out << ").toString8().string())));" << "\n";
    GenerateCodeToStopMeasurement(out);
    out << "cout << \"called\" << endl;" << "\n";

    // Copy the output (call by pointer or reference cases).
    arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg.is_output()) {
        // TODO check the return value
        out << GetConversionToProtobufFunctionName(arg) << "(arg"
            << arg_count << ", "
            << "func_msg->mutable_arg(" << arg_count << "));" << "\n";
      }
      arg_count++;
    }

    out << "return true;" << "\n";
    out.unindent();
    out << "  }" << "\n";
  }
  // TODO: if there were pointers, free them.
  out << "  return false;" << "\n";
  out << "}" << "\n";
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

void HalHidlCodeGen::GenerateHeaderGlobalFunctionDeclarations(
    Formatter& out, const string& function_prototype) {
  out << "extern \"C\" {" << "\n";
  out << "extern " << function_prototype << ";" << "\n";
  out << "}" << "\n";
}

void HalHidlCodeGen::GenerateCppBodyGlobalFunctions(
    Formatter& out, const string& function_prototype,
    const string& fuzzer_extended_class_name) {
  out << "extern \"C\" {" << "\n";
  out << function_prototype << " {" << "\n";
  out << "  return (android::vts::FuzzerBase*) "
      << "new android::vts::" << fuzzer_extended_class_name << "();" << "\n";
  out << "}" << "\n" << "\n";
  out << "}" << "\n";
}

void HalHidlCodeGen::GenerateSubStructFuzzFunctionCall(
    Formatter& out, const StructSpecificationMessage& message,
    const string& parent_path) {
  string current_path(parent_path);
  if (current_path.length() > 0) {
    current_path += ".";
  }
  current_path += message.name();

  string current_path_printable(current_path);
  replace(current_path_printable.begin(), current_path_printable.end(), '.',
          '_');

  out << "    if (func_msg->parent_path() == \"" << current_path << "\") {"
      << "\n";
  out << "      return Fuzz__" << current_path_printable
      << "(func_msg, result, callback_socket_name);" << "\n";
  out << "    }" << "\n";

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateSubStructFuzzFunctionCall(out, sub_struct, current_path);
  }
}

}  // namespace vts
}  // namespace android
