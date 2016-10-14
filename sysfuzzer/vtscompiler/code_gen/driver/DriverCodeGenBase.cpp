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

#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "utils/InterfaceSpecUtil.h"

#include "VtsCompilerUtils.h"

using namespace std;

namespace android {
namespace vts {

void DriverCodeGenBase::GenerateAll(
    std::stringstream& cpp_ss, std::stringstream& h_ss,
    const ComponentSpecificationMessage& message) {
  string input_vfs_file_path(input_vts_file_path_);

  cpp_ss << "#include \"" << input_vfs_file_path << ".h\"" << endl;

  if (message.component_class() == HAL_HIDL) {
    cpp_ss << "#include \""
           << input_vfs_file_path.substr(0, input_vfs_file_path.find_last_of("\\/"))
           << "/types.vts.h\"" << endl;
    cpp_ss << "#include <hidl/HidlSupport.h>" << endl;
  }

  cpp_ss << "#include <iostream>" << endl;
  cpp_ss << "#include \"vts_datatype.h\"" << endl;
  cpp_ss << "#include \"vts_measurement.h\"" << endl;
  for (auto const& header : message.header()) {
    cpp_ss << "#include " << header << endl;
  }
  if (message.component_class() == HAL_HIDL && message.has_component_name()) {
    string package_path = message.package();
    ReplaceSubString(package_path, ".", "/");
    cpp_ss << "#include <" << package_path << "/"
           << GetVersionString(message.component_type_version())
           << "/" << message.component_name() << ".h>" << endl;
    cpp_ss << "#include <" << package_path << "/"
           << GetVersionString(message.component_type_version())
           << "/types.h>" << endl;
    if (message.component_name() != "types") {
      cpp_ss << "#include <" << package_path << "/"
             << GetVersionString(message.component_type_version())
             << "/" << message.component_name() << ".h>" << endl;
    }
    for (const auto& import : message.import()) {
      string mutable_import = import;
      ReplaceSubString(mutable_import, ".", "/");
      ReplaceSubString(mutable_import, "@", "/");
      string base_dirpath = mutable_import.substr(
          0, mutable_import.find_last_of("::") + 1);
      string base_filename = mutable_import.substr(
          mutable_import.find_last_of("::") + 1);
      // TODO: consider restoring this when hidl packaging is fully defined.
      // cpp_ss << "#include <" << base_dirpath << base_filename << ".h>" << endl;
      cpp_ss << "#include <" << package_path << "/"
             << GetVersionString(message.component_type_version())
             << "/" << base_filename << ".h>" << endl;
      if (!endsWith(base_filename, "Callback")) {
        // TODO: ditto
        // cpp_ss << "#include <" << base_dirpath << ...
        if (base_filename != "types") {
          cpp_ss << "#include <" << package_path << "/"
                 << GetVersionString(message.component_type_version())
                 << "/" << base_filename << ".h>" << endl;
        }
      }
      if (base_filename != "types") {
        cpp_ss << "#include <" << package_path << "/"
               << GetVersionString(message.component_type_version())
               << "/" << base_filename << ".h>" << endl;
        if (base_filename.substr(0, 1) == "I") {
          cpp_ss << "#include \""
                 << input_vfs_file_path.substr(0, input_vfs_file_path.find_last_of("\\/"))
                 << "/" << base_filename.substr(1, base_filename.find_last_of(".h"))
                 << ".vts.h\"" << endl;
        }
      }
      cpp_ss << "#include <" << package_path << "/"
             << GetVersionString(message.component_type_version())
             << "/types.h>" << endl;
    }
  }
  GenerateOpenNameSpaces(cpp_ss, message);

  string component_name = GetComponentName(message);
  if (component_name.empty()) {
    cerr << __func__ << ":" << __LINE__ << " error component_name is empty"
         << endl;
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

  GenerateAllHeader(fuzzer_extended_class_name, h_ss, message);
  cpp_ss << endl << endl;
  if (message.component_class() == HAL_CONVENTIONAL ||
      message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
    GenerateCppBodyCallbackFunction(cpp_ss, message,
                                    fuzzer_extended_class_name);
  }

  cpp_ss << endl;
  GenerateCppBodyFuzzFunction(cpp_ss, message, fuzzer_extended_class_name);
  GenerateCppBodyGetAttributeFunction(
      cpp_ss, message, fuzzer_extended_class_name);

  if (message.component_class() == HAL_HIDL &&
      endsWith(message.component_name(), "Callback")) {
    cpp_ss << endl;
    for (const auto& api : message.interface().api()) {
      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        cpp_ss << "Return<void> ";
      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR ||
                 api.return_type_hidl(0).type() == TYPE_ENUM) {
        cpp_ss << "Return<" << api.return_type_hidl(0).scalar_type() << "> ";
      } else {
        cpp_ss << "Status " << endl;
      }

      cpp_ss << "Vts" << message.component_name().substr(1) << "::"
             << api.name() << "(" << endl;
      int arg_count = 0;
      for (const auto& arg : api.arg()) {
        if (arg_count > 0) cpp_ss << "," << endl;
        if (arg.type() == TYPE_ENUM || arg.type() == TYPE_STRUCT) {
          if (arg.is_const()) {
            cpp_ss << "    const " << arg.predefined_type() << "&";
          } else {
            cpp_ss << "    " << arg.predefined_type();
          }
          cpp_ss << " arg" << arg_count;
        } else if (arg.type() == TYPE_VECTOR) {
          cpp_ss << "    const ";
          if (arg.vector_value(0).type() == TYPE_SCALAR) {
            if (arg.vector_value(0).scalar_type().length() == 0) {
              cerr << __func__ << ":" << __LINE__
                   << " ERROR scalar_type not set" << endl;
              exit(-1);
            }
            cpp_ss << "hidl_vec<" + arg.vector_value(0).scalar_type() + ">&";
          } else {
            cerr << __func__ << " unknown vector arg type "
                 << arg.vector_value(0).type() << endl;
            exit(-1);
          }
          cpp_ss << " arg" << arg_count;
        } else if (arg.type() == TYPE_ARRAY) {
          cpp_ss << "    ";
          if (arg.is_const()) {
            cpp_ss << "const ";
          }
          if (arg.vector_value(0).type() == TYPE_SCALAR) {
            cpp_ss << arg.vector_value(0).scalar_type()
                   << "[" << arg.vector_size() << "]";
          } else {
            cerr << __func__ << " unknown vector arg type "
                 << arg.vector_value(0).type() << endl;
            exit(-1);
          }
          cpp_ss << " arg" << arg_count;
        } else {
          cerr << __func__ << " unknown arg type " << arg.type() << endl;
          exit(-1);
        }
        arg_count++;
      }
      cpp_ss << ") {" << endl;
      cpp_ss << "  cout << \"" << api.name() << " called\" << endl;" << endl;
      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        cpp_ss << "  return Void();" << endl;
      } else {
        cpp_ss << "  return Status::ok();" << endl;
      }
      cpp_ss << "}" << endl;
      cpp_ss << endl;
    }

    cpp_ss << "Vts" << message.component_name().substr(1) << "* VtsFuzzerCreate"
           << message.component_name() << "(const string& callback_socket_name)";
    cpp_ss << " {" << endl
           << "  return new Vts" << message.component_name().substr(1) << "();"
           << endl;
    cpp_ss << "}" << endl << endl;
  } else {
    if (message.component_class() != HAL_HIDL ||
        (message.component_name() != "types" &&
         !endsWith(message.component_name(), "Callback"))) {
      std::stringstream ss;
      // return type
      ss << "android::vts::FuzzerBase* " << endl;
      // function name
      string function_name_prefix = GetFunctionNamePrefix(message);
      ss << function_name_prefix << "(" << endl;
      ss << ")";

      GenerateCppBodyGlobalFunctions(cpp_ss, ss.str(),
                                     fuzzer_extended_class_name);
    }
  }

  GenerateCloseNameSpaces(cpp_ss);
}

void DriverCodeGenBase::GenerateAllHeader(
    const string& fuzzer_extended_class_name, std::stringstream& h_ss,
    const ComponentSpecificationMessage& message) {
  h_ss << "#ifndef __VTS_SPEC_" << vts_name_ << "__" << endl;
  h_ss << "#define __VTS_SPEC_" << vts_name_ << "__" << endl;
  h_ss << endl;
  h_ss << "#include <stdio.h>" << endl;
  h_ss << "#include <stdarg.h>" << endl;
  h_ss << "#include <stdlib.h>" << endl;
  h_ss << "#include <string.h>" << endl;
  h_ss << "#define LOG_TAG \"" << fuzzer_extended_class_name << "\"" << endl;
  h_ss << "#include <utils/Log.h>" << endl;
  h_ss << "#include <fuzz_tester/FuzzerBase.h>" << endl;
  h_ss << "#include <fuzz_tester/FuzzerCallbackBase.h>" << endl;
  for (auto const& header : message.header()) {
    h_ss << "#include " << header << endl;
  }
  if (message.component_class() == HAL_HIDL && message.has_component_name()) {
    string package_path = message.package();
    ReplaceSubString(package_path, ".", "/");

    h_ss << "#include <" << package_path << "/"
         << GetVersionString(message.component_type_version())
         << "/" << message.component_name() << ".h>" << endl;
    h_ss << "#include <" << package_path << "/"
         << GetVersionString(message.component_type_version())
         << "/types.h>" << endl;
    if (message.component_name() != "types") {
      h_ss << "#include <" << package_path << "/"
           << GetVersionString(message.component_type_version())
           << "/" << message.component_name() << ".h>" << endl;
    }
    h_ss << "#include <hidl/HidlSupport.h>" << endl;
    h_ss << "#include <hidl/IServiceManager.h>" << endl;
  }
  h_ss << "\n\n" << endl;
  GenerateOpenNameSpaces(h_ss, message);

  GenerateClassHeader(fuzzer_extended_class_name, h_ss, message);

  string function_name_prefix = GetFunctionNamePrefix(message);

  std::stringstream ss;
  // return type
  h_ss << endl;
  ss << "android::vts::FuzzerBase* " << endl;
  // function name
  ss << function_name_prefix << "(" << endl;
  ss << ")";

  if (message.component_class() == HAL_HIDL &&
      message.component_name() != "types" &&
      !endsWith(message.component_name(), "Callback")) {
    GenerateHeaderGlobalFunctionDeclarations(h_ss, ss.str());
  }

  if (message.component_class() == HAL_HIDL &&
      endsWith(message.component_name(), "Callback")) {
    h_ss << endl;
    h_ss << "class Vts" << message.component_name().substr(1) << ": public "
         << message.component_name() << " {" << endl;
    h_ss << " public:" << endl;
    h_ss << "  Vts" << message.component_name().substr(1) << "() {};" << endl;
    h_ss << endl;
    h_ss << "  virtual ~Vts" << message.component_name().substr(1) << "()"
         << " = default;" << endl;
    h_ss << endl;
    for (const auto& api : message.interface().api()) {
      h_ss << "  ";
      if (api.return_type_hidl_size() == 0 ||
          api.return_type_hidl(0).type() == TYPE_VOID) {
        h_ss << "Return<void> ";

      } else if (api.return_type_hidl(0).type() == TYPE_SCALAR ||
                 api.return_type_hidl(0).type() == TYPE_ENUM) {
        h_ss << "Return<" << api.return_type_hidl(0).scalar_type() << "> ";
      } else {
        h_ss << "Status " << endl;
      }

      h_ss << api.name() << "(" << endl;
      int arg_count = 0;
      for (const auto& arg : api.arg()) {
        if (arg_count > 0) h_ss << "," << endl;
        if (arg.type() == TYPE_ENUM || arg.type() == TYPE_STRUCT) {
          if (arg.is_const()) {
            h_ss << "    const " << arg.predefined_type() << "&";
          } else {
            h_ss << "    " << arg.predefined_type();
          }
          h_ss << " arg" << arg_count;
        } else if (arg.type() == TYPE_VECTOR) {
          h_ss << "    const ";
          if (arg.vector_value(0).type() == TYPE_SCALAR) {
            if (arg.vector_value(0).scalar_type().length() == 0) {
              cerr << __func__ << ":" << __LINE__
                   << " ERROR scalar_type not set" << endl;
              exit(-1);
            }
            h_ss << "hidl_vec<" << arg.vector_value(0).scalar_type() << ">&";
          } else {
            cerr << __func__ << " unknown vector arg type "
                 << arg.vector_value(0).type() << endl;
            exit(-1);
          }
          h_ss << " arg" << arg_count;
        } else if (arg.type() == TYPE_ARRAY) {
          h_ss << "    ";
          if (arg.is_const()) {
            h_ss << "const ";
          }
          if (arg.vector_value(0).type() == TYPE_SCALAR) {
            h_ss << arg.vector_value(0).scalar_type()
                 << "[" << arg.vector_size() << "]";
          } else {
            cerr << __func__ << " unknown vector arg type "
                 << arg.vector_value(0).type() << endl;
            exit(-1);
          }
          h_ss << " arg" << arg_count;
        } else {
          cerr << __func__ << " unknown arg type " << arg.type() << endl;
          exit(-1);
        }
        arg_count++;
      }
      h_ss << ") override;" << endl;
      h_ss << endl;
    }
    h_ss << "};" << endl;
    h_ss << endl;

    h_ss << "Vts" << message.component_name().substr(1) << "* VtsFuzzerCreate"
       << message.component_name() << "(const string& callback_socket_name);" << endl;
    h_ss << endl;
  }

  GenerateCloseNameSpaces(h_ss);
  h_ss << "#endif" << endl;
}

void DriverCodeGenBase::GenerateClassHeader(
    const string& fuzzer_extended_class_name, std::stringstream& h_ss,
    const ComponentSpecificationMessage& message) {
  if (message.component_class() != HAL_HIDL ||
      (message.component_name() != "types" &&
       !endsWith(message.component_name(), "Callback"))) {
    h_ss << "class " << fuzzer_extended_class_name << " : public FuzzerBase {"
         << endl;
    h_ss << " public:" << endl;
    h_ss << "  " << fuzzer_extended_class_name << "() : FuzzerBase(";

    if (message.component_class() == HAL_CONVENTIONAL) {
      h_ss << "HAL_CONVENTIONAL)";
    } else if (message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
      h_ss << "HAL_CONVENTIONAL_SUBMODULE)";
    } else if (message.component_class() == HAL_HIDL) {
      if (message.component_name() != "types") {
        h_ss << "HAL_HIDL), hw_binder_proxy_()";
      } else {
        h_ss << "HAL_HIDL)";
      }
    } else if (message.component_class() == HAL_LEGACY) {
      h_ss << "HAL_LEGACY)";
    } else if (message.component_class() == LIB_SHARED) {
      h_ss << "LIB_SHARED)";
    }

    h_ss << " { }" << endl;
    h_ss << " protected:" << endl;
    h_ss << "  bool Fuzz(FunctionSpecificationMessage* func_msg," << endl
         << "            void** result, const string& callback_socket_name);"
         << endl;
    h_ss << "  bool GetAttribute(FunctionSpecificationMessage* func_msg," << endl
         << "            void** result);"
         << endl;

    // produce Fuzz method(s) for sub_struct(s).
    for (auto const& sub_struct : message.interface().sub_struct()) {
      GenerateFuzzFunctionForSubStruct(h_ss, sub_struct, "_");
    }

    if (message.component_class() == HAL_CONVENTIONAL_SUBMODULE) {
      string component_name = GetComponentName(message);
      h_ss << "  void SetSubModule(" << component_name << "* submodule) {"
           << endl;
      h_ss << "    submodule_ = submodule;" << endl;
      h_ss << "  }" << endl;
      h_ss << endl;
      h_ss << " private:" << endl;
      h_ss << "  " << message.original_data_structure_name() << "* submodule_;"
           << endl;
    }

    if (message.component_class() == HAL_HIDL
        && message.component_name() != "types") {
      h_ss << "  bool GetService();" << endl
           << endl
           << " private:" << endl
           << "  sp<" << message.component_name() << "> hw_binder_proxy_;"
           << endl;
    }
    h_ss << "};" << endl;
  }

  if (message.component_class() == HAL_HIDL &&
      message.component_name() == "types") {
    for (int attr_idx = 0;
         attr_idx < message.attribute_size() + message.interface().attribute_size();
         attr_idx++) {
      const auto& attribute = (attr_idx < message.attribute_size()) ?
          message.attribute(attr_idx) :
          message.interface().attribute(attr_idx - message.attribute_size());
      if (attribute.type() == TYPE_ENUM) {
        h_ss << attribute.name() << " " << "Random" << attribute.name() << "();"
               << endl;
      } else if (attribute.type() == TYPE_STRUCT) {
        h_ss << "void " << "MessageTo" << attribute.name()
             << "(const VariableSpecificationMessage& var_msg, "
             << attribute.name() << "* arg);"
             << endl;
      }
    }
  }
}

void DriverCodeGenBase::GenerateFuzzFunctionForSubStruct(
    std::stringstream& h_ss, const StructSpecificationMessage& message,
    const string& parent_path) {
  h_ss << "  bool Fuzz_" << parent_path << message.name()
       << "(FunctionSpecificationMessage* func_msg," << endl;
  h_ss << "            void** result, const string& callback_socket_name);"
       << endl;

  h_ss << "  bool GetAttribute_" << parent_path << message.name()
       << "(FunctionSpecificationMessage* func_msg," << endl;
  h_ss << "            void** result);"
       << endl;

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateFuzzFunctionForSubStruct(h_ss, sub_struct,
                                     parent_path + message.name() + "_");
  }
}

void DriverCodeGenBase::GenerateOpenNameSpaces(
    std::stringstream& ss, const ComponentSpecificationMessage& message) {
  if (message.component_class() == HAL_HIDL && message.has_package()) {
    ss << "using namespace android::hardware;" << endl;
    ss << "using namespace ";
    string name = message.package();
    ReplaceSubString(name, ".", "::");
    ss << name << "::"
       << GetVersionString(message.component_type_version(), true)
       << ";" << endl;
  }

  ss << "namespace android {" << endl;
  ss << "namespace vts {" << endl;
}

void DriverCodeGenBase::GenerateCloseNameSpaces(std::stringstream& ss) {
  ss << "}  // namespace vts" << endl;
  ss << "}  // namespace android" << endl;
}

void DriverCodeGenBase::GenerateCodeToStartMeasurement(std::stringstream& ss) {
  ss << "    VtsMeasurement vts_measurement;" << endl;
  ss << "    vts_measurement.Start();" << endl;
}

void DriverCodeGenBase::GenerateCodeToStopMeasurement(std::stringstream& ss) {
  ss << "    vector<float>* measured = vts_measurement.Stop();" << endl;
  ss << "    cout << \"time \" << (*measured)[0] << endl;" << endl;
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
    std::stringstream& /*cpp_ss*/,
    const ComponentSpecificationMessage& /*message*/,
    const string& /*fuzzer_extended_class_name*/) {}

}  // namespace vts
}  // namespace android
