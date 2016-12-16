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

#include "code_gen/driver/DriverCodeGenBase.h"

#include <hidl-util/Formatter.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "utils/InterfaceSpecUtil.h"

#include "VtsCompilerUtils.h"
#include "utils/StringUtil.h"

using namespace std;

namespace android {
namespace vts {

void DriverCodeGenBase::GenerateAll(
    Formatter& header_out, Formatter& source_out,
    const ComponentSpecificationMessage& message) {
  string component_name = GetComponentName(message);
  if (component_name.empty()) {
    cerr << __func__ << ":" << __LINE__ << " error component_name is empty"
         << "\n";
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
  GenerateHeaderFile(header_out, message, fuzzer_extended_class_name);
  GenerateSourceFile(source_out, message, fuzzer_extended_class_name);
}


void DriverCodeGenBase::GenerateHeaderFile(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  out << "#ifndef __VTS_SPEC_" << vts_name_ << "__" << "\n";
  out << "#define __VTS_SPEC_" << vts_name_ << "__" << "\n";
  out << "\n";
  out << "#include <stdio.h>" << "\n";
  out << "#include <stdarg.h>" << "\n";
  out << "#include <stdlib.h>" << "\n";
  out << "#include <string.h>" << "\n";
  out << "#define LOG_TAG \"" << fuzzer_extended_class_name << "\"" << "\n";
  out << "#include <utils/Log.h>" << "\n";
  out << "#include <fuzz_tester/FuzzerBase.h>" << "\n";
  out << "#include <fuzz_tester/FuzzerCallbackBase.h>" << "\n";
  for (auto const& header : message.header()) {
    out << "#include " << header << "\n";
  }
  if (message.component_class() == HAL_HIDL && message.has_component_name()) {
    string package_path = message.package();
    ReplaceSubString(package_path, ".", "/");

    out << "#include <" << package_path << "/"
        << GetVersionString(message.component_type_version())
        << "/" << message.component_name() << ".h>" << "\n";
    if (message.component_name() != "types") {
      out << "#include <" << package_path << "/"
          << GetVersionString(message.component_type_version())
          << "/" << message.component_name() << ".h>" << "\n";
    }
    out << "#include <hidl/HidlSupport.h>" << "\n";
  }
  out << "\n\n" << "\n";
  GenerateOpenNameSpaces(out, message);

  GenerateClassHeader(fuzzer_extended_class_name, out, message);

  string function_name_prefix = GetFunctionNamePrefix(message);

  std::stringstream ss;
  // return type
  out << "\n";
  ss << "android::vts::FuzzerBase* " << "\n";
  // function name
  ss << function_name_prefix << "(" << "\n";
  ss << ")";

  if (message.component_class() == HAL_HIDL &&
      message.component_name() != "types" &&
      !endsWith(message.component_name(), "Callback")) {
    GenerateHeaderGlobalFunctionDeclarations(out, ss.str());
  }

  if (message.component_class() == HAL_HIDL &&
      endsWith(message.component_name(), "Callback")) {
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
      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        out << "::android::hardware::Return<void> ";

      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR ||
                 api.return_type_hidl(0).type() == TYPE_ENUM) {
        out << "Return<" << api.return_type_hidl(0).scalar_type() << "> ";
      } else {
        out << "Status " << "\n";
      }

      out << api.name() << "(" << "\n";
      int arg_count = 0;
      for (const auto& arg : api.arg()) {
        if (arg_count > 0) out << "," << "\n";
        if (arg.type() == TYPE_ENUM) {
          if (arg.is_const()) {
            out << "    const " << arg.predefined_type() << "&";
          } else {
            out << "    " << arg.predefined_type();
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
                << arg.vector_value(0).scalar_type()
                << ">&";
          } else {
            cerr << __func__ << " unknown vector arg type "
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
            out << arg.vector_value(0).scalar_type()
                << "[" << arg.vector_size() << "]";
          } else {
            cerr << __func__ << " unknown vector arg type "
                 << arg.vector_value(0).type() << "\n";
            exit(-1);
          }
          out << " arg" << arg_count;
        } else {
          cerr << __func__ << " unknown arg type " << arg.type() << "\n";
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

    out << "Vts" << message.component_name().substr(1) << "* VtsFuzzerCreate"
        << message.component_name() << "(const string& callback_socket_name);" << "\n";
    out << "\n";
  }

  GenerateCloseNameSpaces(out);
  out << "#endif" << "\n";
}


void DriverCodeGenBase::GenerateSourceFile(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  string input_vfs_file_path(input_vts_file_path_);

  out << "#include \"" << input_vfs_file_path << ".h\"" << "\n";

  if (message.component_class() == HAL_HIDL) {

    out << "#include <hidl/HidlSupport.h>" << "\n";
  }

  out << "#include <iostream>" << "\n";
  out << "#include \"vts_datatype.h\"" << "\n";
  out << "#include \"vts_measurement.h\"" << "\n";
  for (auto const& header : message.header()) {
    out << "#include " << header << "\n";
  }
  if (message.component_class() == HAL_HIDL && message.has_component_name()) {
    out << "#include <hidl/HidlSupport.h>" << "\n";

    string package_path = message.package();
    ReplaceSubString(package_path, ".", "/");
    out << "#include <" << package_path << "/"
        << GetVersionString(message.component_type_version())
        << "/" << message.component_name() << ".h>" << "\n";
    for (const auto& import : message.import()) {
      string mutable_import = import;
      ReplaceSubString(mutable_import, ".", "/");
      ReplaceSubString(mutable_import, "@", "/");
      string base_dirpath = mutable_import.substr(
          0, mutable_import.find_last_of("::") + 1);
      string base_filename = mutable_import.substr(
          mutable_import.find_last_of("::") + 1);

      if (base_filename == "types") {
        out << "#include \""
            << input_vfs_file_path.substr(
                0, input_vfs_file_path.find_last_of("\\/"))
            << "/types.vts.h\"" << "\n";
      }
      if (base_filename != "types") {
        if (message.component_name() != base_filename) {
          out << "#include <" << package_path << "/"
              << GetVersionString(message.component_type_version())
              << "/" << base_filename << ".h>" << "\n";
        }
        if (base_filename.substr(0, 1) == "I") {
          out << "#include \""
              << input_vfs_file_path.substr(0, input_vfs_file_path.find_last_of("\\/"))
              << "/" << base_filename.substr(1, base_filename.find_last_of(".h"))
              << ".vts.h\"" << "\n";
        }
      } else if (message.component_name() != base_filename) {
        // TODO: consider restoring this when hidl packaging is fully defined.
        // cpp_ss << "#include <" << base_dirpath << base_filename << ".h>" << "\n";
        out << "#include <" << package_path << "/"
            << GetVersionString(message.component_type_version())
            << "/" << base_filename << ".h>" << "\n";
      }
    }
  }
  GenerateOpenNameSpaces(out, message);

  out << "\n" << "\n";
  if (message.component_class() == HAL_CONVENTIONAL ||
      message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
    GenerateCppBodyCallbackFunction(out, message,
                                    fuzzer_extended_class_name);
  }

  out << "\n";
  GenerateCppBodyFuzzFunction(out, message, fuzzer_extended_class_name);
  GenerateCppBodyGetAttributeFunction(
      out, message, fuzzer_extended_class_name);

  if (message.component_class() == HAL_HIDL &&
      endsWith(message.component_name(), "Callback")) {
    out << "\n";
    for (const auto& api : message.interface().api()) {
      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        out << "::android::hardware::Return<void> ";
      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR ||
                 api.return_type_hidl(0).type() == TYPE_ENUM) {
        out << "Return<" << api.return_type_hidl(0).scalar_type() << "> ";
      } else {
        out << "Status " << "\n";
      }

      out << "Vts" << message.component_name().substr(1) << "::"
          << api.name() << "(" << "\n";
      int arg_count = 0;
      for (const auto& arg : api.arg()) {
        if (arg_count > 0) out << "," << "\n";
        if (arg.type() == TYPE_ENUM) {
          if (arg.is_const()) {
            out << "    const " << arg.predefined_type() << "&";
          } else {
            out << "    " << arg.predefined_type();
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
                << arg.vector_value(0).scalar_type()
                << ">&";
          } else {
            cerr << __func__ << " unknown vector arg type "
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
            out << arg.vector_value(0).scalar_type()
                << "[" << arg.vector_size() << "]";
          } else {
            cerr << __func__ << " unknown vector arg type "
                 << arg.vector_value(0).type() << "\n";
            exit(-1);
          }
          out << " arg" << arg_count;
        } else {
          cerr << __func__ << " unknown arg type " << arg.type() << "\n";
          exit(-1);
        }
        arg_count++;
      }
      out << ") {" << "\n";
      out.indent();
      out << "cout << \"" << api.name() << " called\" << endl;" << "\n";
      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        out << "return ::android::hardware::Void();" << "\n";
      } else {
        out << "return Status::ok();" << "\n";
      }
      out.unindent();
      out << "}" << "\n";
      out << "\n";
    }

    out << "Vts" << message.component_name().substr(1) << "* VtsFuzzerCreate"
        << message.component_name() << "(const string& callback_socket_name)";
    out << " {" << "\n";
    out.indent();
    out << "return new Vts" << message.component_name().substr(1) << "();"
        << "\n";
    out.unindent();
    out << "}" << "\n" << "\n";
  } else {
    if (message.component_class() != HAL_HIDL ||
        (message.component_name() != "types" &&
         !endsWith(message.component_name(), "Callback"))) {
      std::stringstream ss;
      // return type
      ss << "android::vts::FuzzerBase* " << "\n";
      // function name
      string function_name_prefix = GetFunctionNamePrefix(message);
      ss << function_name_prefix << "(" << "\n";
      ss << ")";

      GenerateCppBodyGlobalFunctions(out, ss.str(), fuzzer_extended_class_name);
    }
  }

  GenerateCloseNameSpaces(out);
}


void DriverCodeGenBase::GenerateClassHeader(
    const string& fuzzer_extended_class_name, Formatter& out,
    const ComponentSpecificationMessage& message) {
  if (message.component_class() != HAL_HIDL ||
      (message.component_name() != "types" &&
       !endsWith(message.component_name(), "Callback"))) {
    out << "class " << fuzzer_extended_class_name << " : public FuzzerBase {"
        << "\n";
    out << " public:" << "\n";
    out.indent();
    out << fuzzer_extended_class_name << "() : FuzzerBase(";

    if (message.component_class() == HAL_CONVENTIONAL) {
      out << "HAL_CONVENTIONAL)";
    } else if (message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
      out << "HAL_CONVENTIONAL_SUBMODULE)";
    } else if (message.component_class() == HAL_HIDL) {
      if (message.component_name() != "types") {
        out << "HAL_HIDL), hw_binder_proxy_()";
      } else {
        out << "HAL_HIDL)";
      }
    } else if (message.component_class() == HAL_LEGACY) {
      out << "HAL_LEGACY)";
    } else if (message.component_class() == LIB_SHARED) {
      out << "LIB_SHARED)";
    }

    out << " { }" << "\n";
    out.unindent();
    out << " protected:" << "\n";
    out.indent();
    out << "bool Fuzz(FunctionSpecificationMessage* func_msg," << "\n"
        << "          void** result, const string& callback_socket_name);"
        << "\n";
    out << "bool GetAttribute(FunctionSpecificationMessage* func_msg," << "\n"
        << "          void** result);"
        << "\n";

    // produce Fuzz method(s) for sub_struct(s).
    for (auto const& sub_struct : message.interface().sub_struct()) {
      GenerateFuzzFunctionForSubStruct(out, sub_struct, "_");
    }

    if (message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
      string component_name = GetComponentName(message);
      out << "void SetSubModule(" << component_name << "* submodule) {"
          << "\n";
      out.indent();
      out << "submodule_ = submodule;" << "\n";
      out.unindent();
      out << "}" << "\n" << "\n";
      out.unindent();
      out << " private:" << "\n";
      out.indent();
      out << message.original_data_structure_name() << "* submodule_;" << "\n";
    }

    if (message.component_class() == HAL_HIDL
        && message.component_name() != "types") {
      out << "bool GetService(bool get_stub);" << "\n"
          << "\n";
      out.unindent();
      out << " private:" << "\n";
      out.indent();
      out << "sp<" << message.component_name() << "> hw_binder_proxy_;"
          << "\n";
    }
    out.unindent();
    out << "};" << "\n";
  }

  if (message.component_class() == HAL_HIDL &&
      message.component_name() == "types") {
    for (int attr_idx = 0;
         attr_idx < message.attribute_size() + message.interface().attribute_size();
         attr_idx++) {
      const auto& attribute = (attr_idx < message.attribute_size()) ?
          message.attribute(attr_idx) :
          message.interface().attribute(attr_idx - message.attribute_size());
      std::string attribute_name = attribute.name();
      ReplaceSubString(attribute_name, "::", "__");
      if (attribute.type() == TYPE_ENUM) {
        out << attribute.name() << " "
            << "Random" << attribute_name << "();"
            << "\n";
      } else if (attribute.type() == TYPE_STRUCT) {
        std::string attribute_name = attribute.name();
        ReplaceSubString(attribute_name, "::", "__");
        out << "void " << "MessageTo" << attribute_name
            << "(const VariableSpecificationMessage& var_msg, "
            << attribute.name() << "* arg);"
            << "\n";
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
             << attribute.type() << endl;
        exit(-1);
      }
    }
  }
}

void DriverCodeGenBase::GenerateFuzzFunctionForSubStruct(
    Formatter& out, const StructSpecificationMessage& message,
    const string& parent_path) {
  out.indent();
  out << "bool Fuzz_" << parent_path << message.name()
      << "(FunctionSpecificationMessage* func_msg," << "\n";
  out << "            void** result, const string& callback_socket_name);"
      << "\n";

  out << "bool GetAttribute_" << parent_path << message.name()
      << "(FunctionSpecificationMessage* func_msg," << "\n";
  out << "            void** result);"
      << "\n";

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateFuzzFunctionForSubStruct(out, sub_struct,
                                     parent_path + message.name() + "_");
  }
  out.unindent();
}

void DriverCodeGenBase::GenerateNamespaceName(
    Formatter& out, const ComponentSpecificationMessage& message) {
  if (message.component_class() == HAL_HIDL && message.has_package()) {
    string name = message.package();
    ReplaceSubString(name, ".", "::");
    out << name << "::"
        << GetVersionString(message.component_type_version(), true);
  } else {
    cerr << __func__ << ":" << __LINE__ << " no namespace" << "\n";
    exit(-1);
  }
}

void DriverCodeGenBase::GenerateOpenNameSpaces(
    Formatter& out, const ComponentSpecificationMessage& message) {
  if (message.component_class() == HAL_HIDL && message.has_package()) {
    out << "using namespace ";
    GenerateNamespaceName(out, message);
    out << ";" << "\n";
  }

  out << "namespace android {" << "\n";
  out << "namespace vts {" << "\n";
}

void DriverCodeGenBase::GenerateCloseNameSpaces(Formatter& out) {
  out << "}  // namespace vts" << "\n";
  out << "}  // namespace android" << "\n";
}

void DriverCodeGenBase::GenerateCodeToStartMeasurement(Formatter& out) {
  out << "VtsMeasurement vts_measurement;" << "\n";
  out << "vts_measurement.Start();" << "\n";
}

void DriverCodeGenBase::GenerateCodeToStopMeasurement(Formatter& out) {
  out << "vector<float>* measured = vts_measurement.Stop();" << "\n";
  out << "cout << \"time \" << (*measured)[0] << endl;" << "\n";
}

string DriverCodeGenBase::GetComponentName(
    const ComponentSpecificationMessage& message) {
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

void DriverCodeGenBase::GenerateCppBodyCallbackFunction(
    Formatter& /*out*/,
    const ComponentSpecificationMessage& /*message*/,
    const string& /*fuzzer_extended_class_name*/) {}

}  // namespace vts
}  // namespace android
