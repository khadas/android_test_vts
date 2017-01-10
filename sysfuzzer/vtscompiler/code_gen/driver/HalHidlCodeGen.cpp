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
      if (api.return_type_hidl_size() == 0
          || api.return_type_hidl(0).type() == TYPE_VOID) {
        out << "::android::hardware::Return<void> ";
      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR
          || api.return_type_hidl(0).type() == TYPE_ENUM) {
        out << "Return<" << api.return_type_hidl(0).scalar_type() << "> ";
      } else {
        out << "Status " << "\n";
      }

      out << "Vts" << message.component_name().substr(1) << "::" << api.name()
          << "(" << "\n";
      int arg_count = 0;
      for (const auto& arg : api.arg()) {
        if (arg_count > 0)
          out << "," << "\n";
        if (arg.type() == TYPE_ENUM) {
          if (arg.is_const()) {
            out << "    const " << arg.predefined_type() << "&";
          } else {
            out << "    " << arg.predefined_type();
          }
          out << " arg" << arg_count;
        } else if (arg.type() == TYPE_SCALAR) {
          if (arg.is_const()) {
            out << "    const " << arg.scalar_type() << "&";
          } else {
            out << "    " << arg.scalar_type();
          }
          out << " arg" << arg_count;
        } else if (arg.type() == TYPE_STRUCT) {
          out << "    const " << arg.predefined_type() << "&";
          out << " arg" << arg_count;
        } else if (arg.type() == TYPE_VECTOR) {
          out << "    const ";
          if (arg.vector_value(0).type() == TYPE_SCALAR) {
            if (arg.vector_value(0).scalar_type().length() == 0) {
              cerr << __func__ << ":" << __LINE__
                  << " ERROR scalar_type not set" << "\n";
              exit(-1);
            }
            out << "::android::hardware::hidl_vec<"
                << arg.vector_value(0).scalar_type() << ">&";
          } else if (arg.vector_value(0).type() == TYPE_STRUCT
              || arg.vector_value(0).type() == TYPE_ENUM) {
            out << "::android::hardware::hidl_vec<"
                << arg.vector_value(0).predefined_type() << ">&";
          } else {
            cerr << __func__ << ":" << __LINE__ << " unknown vector arg type "
                << arg.vector_value(0).type() << "\n";
            exit(-1);
          }
          out << " arg" << arg_count;
        } else if (arg.type() == TYPE_ARRAY) {
          out << "    ";
          if (arg.is_const()) {
            out << "const ";
          }
          if (arg.vector_value(0).type() == TYPE_SCALAR) {
            out << arg.vector_value(0).scalar_type() << "[" << arg.vector_size()
                << "]";
          } else {
            cerr << __func__ << " unknown vector arg type "
                << arg.vector_value(0).type() << "\n";
            exit(-1);
          }
          out << " arg" << arg_count;
        } else {
          cerr << __func__ << ":" << __LINE__ << " unknown arg type "
              << arg.type() << "\n";
          exit(-1);
        }
        arg_count++;
      }
      out << ") {" << "\n";
      out.indent();
      out << "cout << \"" << api.name() << " called\" << endl;" << "\n";
      if (api.return_type_hidl_size() == 0
          || api.return_type_hidl(0).type() == TYPE_VOID) {
        out << "return ::android::hardware::Void();" << "\n";
      } else {
        out << "return Status::ok();" << "\n";
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

void HalHidlCodeGen::GenerateCppBodySyncCallbackFunction(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  for (auto const& api : message.interface().api()) {
    if (api.return_type_hidl_size() > 0) {
      out << "static void " << fuzzer_extended_class_name << api.name()
          << "_cb_func(";
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
        } else if (return_type_hidl.type() == TYPE_ENUM
            || return_type_hidl.type() == TYPE_VECTOR) {
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
        } else if (return_type_hidl.type() == TYPE_HANDLE) {
          out << "const ::android::hardware::hidl_handle& ";
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
      out << "  cout << \"callback " << api.name() << " called\"" << " << endl;"
          << "\n";
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
        } else if (return_type_hidl.type() == TYPE_ENUM
            || return_type_hidl.type() == TYPE_VECTOR
            || return_type_hidl.type() == TYPE_STRUCT) {
          out << GetCppVariableType(return_type_hidl, &message);
        } else if (return_type_hidl.type() == TYPE_STRING) {
          out << "::android::hardware::hidl_string";
        } else if (return_type_hidl.type() == TYPE_HANDLE) {
          out << "const ::android::hardware::hidl_handle& ";
        } else {
          cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
              << return_type_hidl.type() << " for " << api.name() << "\n";
          exit(-1);
        }
      }
      out << ")> " << fuzzer_extended_class_name << api.name() << "_cb = "
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
          } else if (arg.type() == TYPE_HIDL_INTERFACE) {
            out << "/* TYPE_HIDL_INTERFACE not supported yet */\n";
            out << GetCppVariableType(arg, &message) << " ";
            out << "arg" << arg_count;
          } else {
            out << GetCppVariableType(arg, &message) << " ";
            out << "arg" << arg_count << " = ";
          }

          if (arg.type() != TYPE_VECTOR &&
              arg.type() != TYPE_HIDL_CALLBACK &&
              arg.type() != TYPE_STRUCT &&
              arg.type() != TYPE_HIDL_INTERFACE) {
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
                  << "scalar_value());"
                  << "\n";
            } else if (arg.vector_value(0).type() == TYPE_STRUCT) {
              out << "/* arg" << arg_count << "buffer[vector_index] not initialized "
                  << "since TYPE_STRUCT not yet supported */" << "\n";
            } else if (arg.vector_value(0).type() == TYPE_STRING) {
              out << "/* arg" << arg_count << "buffer[vector_index] not initialized "
                  << "since TYPE_STRING not yet supported */" << "\n";
            } else {
              cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
                   << arg.vector_value(0).type() << "\n";
              exit(-1);
            }
            out.unindent();
            out << "}" << "\n";
            if (arg.vector_value(0).type() == TYPE_SCALAR
                || arg.vector_value(0).type() == TYPE_ENUM) {
              out << "arg" << arg_count << ".setToExternal(" << "arg"
                  << arg_count << "buffer, " << "func_msg->arg(" << arg_count
                  << ").vector_size()" << ")";
            }
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
            << "(const ScalarDataValueMessage& arg) {\n";
        out.indent();
        out << "return (" << attribute.name() << ") arg."
            << attribute.enum_value().scalar_type() << "();\n";
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
                      << sub_value_index << ").scalar_value());" << "\n";
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
            } else if (struct_value.vector_value(0).type() == TYPE_ENUM) {
              std::string enum_attribute_name =
                  struct_value.vector_value(0).predefined_type();
              out << "for (int value_index = 0; value_index < "
                  << "var_msg.struct_value(" << struct_index
                  << ").vector_size(); "
                  << "value_index++) {" << "\n";
              out.indent();
              out << "arg->" << struct_value.name() << "[value_index] = "
                  << "EnumValue" << enum_attribute_name << "("
                  << "var_msg.struct_value(" << struct_index
                  << ").vector_value(value_index).scalar_value());" << "\n";
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
                << ").scalar_value());" << "\n";
          } else if (struct_value.type() == TYPE_STRUCT) {
            if (struct_value.has_predefined_type()) {
              std::string struct_attribute_name = struct_value.predefined_type();
              ReplaceSubString(struct_attribute_name, "::", "__");
              // TODO(yim): support struct type.
              out << "/*" << struct_value.predefined_type() << "*/";
            } else {
              cerr << __func__ << ":" << __LINE__ << " ERROR predefined_type "
                   << "not defined." << "\n";
              exit(-1);
            }
          } else if (struct_value.type() == TYPE_UNION) {
            if (struct_value.has_predefined_type()) {
              std::string struct_attribute_name = struct_value.predefined_type();
              ReplaceSubString(struct_attribute_name, "::", "__");
              /*
               * TODO(yim): support union which is defined inside a struct.
               * A function needs to be auto-generated which can the pointer to
               * a union instance and assign values in a recursive way.
               * for (const auto& union_value : struct_value.union_value()) {
               *  out << "arg->" << struct_value.name() << "." << union_value.name() << " = "
               *      << "EnumValue" << enum_attribute_name << "("
               *      << "var_msg.struct_value(" << struct_index
               *      << ").enum_value());" << "\n";
               * }
               */
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

void HalHidlCodeGen::GenerateCppBodyFuzzFunction(Formatter& out,
    const StructSpecificationMessage& message,
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
    if ((parent_path_printable + message.name()) == "_common_methods"
        && api.name() == "open") {
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
          if (arg.scalar_type() == "pointer"
              || arg.scalar_type() == "char_pointer"
              || arg.scalar_type() == "void_pointer"
              || arg.scalar_type() == "function_pointer") {
            out << ".has_pointer())? ";
            out << "reinterpret_cast<" << GetCppVariableType(arg) << ">";
          } else {
            out << ".has_" << arg.scalar_type() << "())? ";
          }
          out << "(" << msg << ".scalar_value()";

          if (arg.scalar_type() == "bool_t" || arg.scalar_type() == "int32_t"
              || arg.scalar_type() == "uint32_t"
              || arg.scalar_type() == "int64_t"
              || arg.scalar_type() == "uint64_t"
              || arg.scalar_type() == "int16_t"
              || arg.scalar_type() == "uint16_t"
              || arg.scalar_type() == "int8_t" || arg.scalar_type() == "uint8_t"
              || arg.scalar_type() == "float_t"
              || arg.scalar_type() == "double_t") {
            out << "." << arg.scalar_type() << "() ";
          } else if (arg.scalar_type() == "pointer"
              || arg.scalar_type() == "char_pointer"
              || arg.scalar_type() == "function_pointer"
              || arg.scalar_type() == "void_pointer") {
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
    out << "*result = const_cast<void*>(reinterpret_cast<const void*>"
        << "(new string(" << kInstanceVariableName << "->" << parent_path
        << message.name() << "->" << api.name() << "(";
    if (arg_count > 0)
      out << "\n";

    out.indent();
    for (int index = 0; index < arg_count; index++) {
      out << "arg" << index;
      if (index != (arg_count - 1)) {
        out << "," << "\n";
      }
    }
    out.unindent();

    if (api.return_type_hidl_size() > 0) {
      if (arg_count != 0)
        out << ", ";
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
        out << GetConversionToProtobufFunctionName(arg) << "(arg" << arg_count
            << ", " << "func_msg->mutable_arg(" << arg_count << "));" << "\n";
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
  out << "cout << \"local_device = \" << " << kInstanceVariableName << ".get();"
      << "\n";

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
  if (func_msg.return_type_hidl_size() == 0
      || func_msg.return_type_hidl(0).type() == TYPE_VOID
      || CanElideCallback(func_msg)) {
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
    if (return_val.type() == TYPE_SCALAR) {
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

void HalHidlCodeGen::GenerateClassHeader(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  if (message.component_name() != "types"
      && !endsWith(message.component_name(), "Callback")) {
    DriverCodeGenBase::GenerateClassHeader(out, message,
                                           fuzzer_extended_class_name);
  } else if (message.component_name() == "types") {
    for (int attr_idx = 0;
        attr_idx
            < message.attribute_size() + message.interface().attribute_size();
        attr_idx++) {
      const auto& attribute =
          (attr_idx < message.attribute_size()) ?
              message.attribute(attr_idx) :
              message.interface().attribute(
                  attr_idx - message.attribute_size());
      std::string attribute_name = attribute.name();
      ReplaceSubString(attribute_name, "::", "__");
      if (attribute.type() == TYPE_ENUM) {
        out << attribute.name() << " " << "EnumValue" << attribute_name
            << "(const ScalarDataValueMessage& arg);\n";
        out << "\n";
        out << attribute.name() << " " << "Random" << attribute_name << "();"
            << "\n";
      } else if (attribute.type() == TYPE_STRUCT
          || attribute.type() == TYPE_UNION) {
        std::string attribute_name = attribute.name();
        ReplaceSubString(attribute_name, "::", "__");
        out << "void " << "MessageTo" << attribute_name
            << "(const VariableSpecificationMessage& var_msg, "
            << attribute.name() << "* arg);" << "\n";
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
            << attribute.type() << endl;
        exit(-1);
      }
    }
    for (const auto attribute: message.attribute()){
       GenerateVerificationDeclForAttribute(out, attribute);
       GenerateSetResultDeclForAttribute(out,attribute);
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
      if (api.return_type_hidl_size() == 0
          || api.return_type_hidl(0).type() == TYPE_VOID) {
        out << "::android::hardware::Return<void> ";

      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR
          || api.return_type_hidl(0).type() == TYPE_ENUM) {
        out << "Return<" << api.return_type_hidl(0).scalar_type() << "> ";
      } else {
        out << "Status " << "\n";
      }

      out << api.name() << "(" << "\n";
      int arg_count = 0;
      for (const auto& arg : api.arg()) {
        if (arg_count > 0)
          out << "," << "\n";
        if (arg.type() == TYPE_ENUM) {
          if (arg.is_const()) {
            out << "    const " << arg.predefined_type() << "&";
          } else {
            out << "    " << arg.predefined_type();
          }
          out << " arg" << arg_count;
        } else if (arg.type() == TYPE_SCALAR) {
          if (arg.is_const()) {
            out << "    const " << arg.scalar_type() << "&";
          } else {
            out << "    " << arg.scalar_type();
          }
        } else if (arg.type() == TYPE_STRUCT) {
          out << "    const " << arg.predefined_type() << "&";
          out << " arg" << arg_count;
        } else if (arg.type() == TYPE_VECTOR) {
          out << "    const ";
          if (arg.vector_value(0).type() == TYPE_SCALAR) {
            if (arg.vector_value(0).scalar_type().length() == 0) {
              cerr << __func__ << ":" << __LINE__
                  << " ERROR scalar_type not set" << "\n";
              exit(-1);
            }
            out << "::android::hardware::hidl_vec<"
                << arg.vector_value(0).scalar_type() << ">&";
          } else if (arg.vector_value(0).type() == TYPE_STRUCT
              || arg.vector_value(0).type() == TYPE_ENUM) {
            if (arg.vector_value(0).predefined_type().length() == 0) {
              cerr << __func__ << ":" << __LINE__
                  << " ERROR predefined_type not set" << "\n";
              exit(-1);
            }
            out << "::android::hardware::hidl_vec<"
                << arg.vector_value(0).predefined_type() << ">&";
          } else {
            cerr << __func__ << ":" << __LINE__ << " unknown vector arg type "
                << arg.vector_value(0).type() << "\n";
            exit(-1);
          }
          out << " arg" << arg_count;
        } else if (arg.type() == TYPE_ARRAY) {
          out << "    ";
          if (arg.is_const()) {
            out << "const ";
          }
          if (arg.vector_value(0).type() == TYPE_SCALAR) {
            out << arg.vector_value(0).scalar_type() << "[" << arg.vector_size()
                << "]";
          } else {
            cerr << __func__ << " unknown vector arg type "
                << arg.vector_value(0).type() << "\n";
            exit(-1);
          }
          out << " arg" << arg_count;
        } else {
          cerr << __func__ << ":" << __LINE__ << " unknown arg type "
              << arg.type() << "\n";
          exit(-1);
        }
        arg_count++;
      }
      out << ") override;" << "\n";
      out << "\n";
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
    GenerateCppBodySyncCallbackFunction(out, message,
                                        fuzzer_extended_class_name);
    GenerateGetServiceImpl(out, message, fuzzer_extended_class_name);
    DriverCodeGenBase::GenerateClassImpl(out, message,
                                         fuzzer_extended_class_name);
    for (auto attribute : message.attribute()) {
      GenerateDriverImplForAttribute(out, attribute);
      GenerateRandomFunctionForAttribute(out, attribute);
      GenerateVerificationImplForAttribute(out, attribute);
      GenerateSetResultImplForAttribute(out, attribute);
    }
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
    out << "bool GetService(bool get_stub, const char* service_name);" << "\n\n";
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
        GenerateDriverImplForTypedVariable(
            out, union_value, "arg->" + union_value.name(),
            "var_msg.union_value(" + std::to_string(union_index) + ")");
        union_index++;
      }
      out.unindent();
      out << "}\n";
      break;
    }
    default:
    {
      cerr << __func__ << " unsupported attribute type " << attribute.type()
          << "\n";
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
  if (func_msg.return_type_hidl(0).type() == TYPE_SCALAR
      || func_msg.return_type_hidl(0).type() == TYPE_ENUM) {
    return true;
  }
  return false;
}

}  // namespace vts
}  // namespace android
