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

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <fstream>

#include "specification_parser/InterfaceSpecificationParser.h"
#include "utils/InterfaceSpecUtil.h"

#include "test/vts/sysfuzzer/common/proto/InterfaceSpecificationMessage.pb.h"


using namespace std;


namespace android {
namespace vts {

int vts_fs_mkdirs(const char* file_path, mode_t mode) {
  char* p;

  for (p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
    *p = '\0';
    if (mkdir(file_path, mode) == -1) {
      if (errno != EEXIST) {
        *p = '/';
        return -1;
      }
    }
    *p='/';
  }
  return 0;
}


string ComponentClassToString(int component_class) {
  switch(component_class) {
    case UNKNOWN_CLASS: return "unknown_class";
    case HAL: return "hal";
    case SHAREDLIB: return "sharedlib";
    case HAL_HIDL: return "hal_hidl";
    case HAL_SUBMODULE: return "hal_submodule";
  }
  cerr << "invalid component_class " << component_class << endl;
  exit(-1);
}


string ComponentTypeToString(int component_type) {
  switch(component_type) {
    case UNKNOWN_TYPE: return "unknown_type";
    case AUDIO: return "audio";
    case CAMERA: return "camera";
    case GPS: return "gps";
    case LIGHT: return "light";
  }
  cerr << "invalid component_type " << component_type << endl;
  exit(-1);
}

// Returns the C/C++ variable type name of a given data type.
string GetCppVariableType(const std::string primitive_type_string) {
  const char* primitive_type = primitive_type_string.c_str();
  if (!strcmp(primitive_type, "void")
      || !strcmp(primitive_type, "bool")
      || !strcmp(primitive_type, "int32_t")
      || !strcmp(primitive_type, "uint32_t")
      || !strcmp(primitive_type, "int8_t")
      || !strcmp(primitive_type, "uint8_t")
      || !strcmp(primitive_type, "int64_t")
      || !strcmp(primitive_type, "uint64_t")
      || !strcmp(primitive_type, "int16_t")
      || !strcmp(primitive_type, "uint16_t")
      || !strcmp(primitive_type, "float")
      || !strcmp(primitive_type, "double")) {
    return primitive_type_string;
  } else if(!strcmp(primitive_type, "ufloat")) {
    return "unsigned float";
  } else if(!strcmp(primitive_type, "udouble")) {
    return "unsigned double";
  } else if (!strcmp(primitive_type, "string")) {
    return "std::string";
  } else if (!strcmp(primitive_type, "pointer")) {
    return "void*";
  } else if (!strcmp(primitive_type, "char_pointer")) {
    return "char*";
  } else if (!strcmp(primitive_type, "uchar_pointer")) {
    return "unsigned char*";
  } else if (!strcmp(primitive_type, "void_pointer")) {
    return "void*";
  } else if (!strcmp(primitive_type, "function_pointer")) {
    return "void*";
  }

  cerr << __FILE__ << ":" << __LINE__ << " "
      << "unknown primitive_type " << primitive_type << endl;
  exit(-1);
}


// Returns the C/C++ basic variable type name of a given argument.
string GetCppVariableType(ArgumentSpecificationMessage arg) {
  if (arg.has_aggregate_type()) {
    return arg.aggregate_type();
  } else if (arg.has_primitive_type()) {
    return GetCppVariableType(arg.primitive_type());
  }
  cerr << __FUNCTION__ << ": neither instance nor data type is set" << endl;
  exit(-1);
}


// Get the C/C++ instance type name of an argument.
string GetCppInstanceType(ArgumentSpecificationMessage arg) {
  if (arg.has_aggregate_type()) {
    if (!strcmp(arg.aggregate_type().c_str(), "struct light_state_t*")) {
      return "GenerateLightState()";
    } else if (!strcmp(arg.aggregate_type().c_str(), "GpsCallbacks*")) {
      return "GenerateGpsCallbacks()";
    } else if (!strcmp(arg.aggregate_type().c_str(), "GpsUtcTime")) {
      return "GenerateGpsUtcTime()";
    } else if (!strcmp(arg.aggregate_type().c_str(), "vts_gps_latitude")) {
      return "GenerateLatitude()";
    } else if (!strcmp(arg.aggregate_type().c_str(), "vts_gps_longitude")) {
      return "GenerateLongitude()";
    } else if (!strcmp(arg.aggregate_type().c_str(), "vts_gps_accuracy")) {
      return "GenerateGpsAccuracy()";
    } else if (!strcmp(arg.aggregate_type().c_str(), "vts_gps_flags_uint16")) {
      return "GenerateGpsFlagsUint16()";
    } else if (!strcmp(arg.aggregate_type().c_str(), "GpsPositionMode")) {
      return "GenerateGpsPositionMode()";
    } else if (!strcmp(arg.aggregate_type().c_str(), "GpsPositionRecurrence")) {
      return "GenerateGpsPositionRecurrence()";
    } else {
      cerr << __FILE__ << ":" << __LINE__ << " "
          << "unknown instance type " << arg.aggregate_type() << endl;
    }
  }
  if (arg.has_primitive_type()) {
    if (!strcmp(arg.primitive_type().c_str(), "uint32_t")) {
      return "RandomUint32()";
    } else if (!strcmp(arg.primitive_type().c_str(), "int32_t")) {
      return "RandomInt32()";
    } else if (!strcmp(arg.primitive_type().c_str(), "char_pointer")) {
      return "RandomCharPointer()";
    }
    cerr << __FILE__ << ":" << __LINE__ << " "
        << "unknown data type " << arg.primitive_type() << endl;
    exit(-1);
  }
  cerr << __FUNCTION__ << ": neither instance nor data type is set" << endl;
  exit(-1);
}


// Translates the vts proto file to C/C++ code and header files.
void Translate(
    const char* input_vts_file_path,
    const char* output_header_dir_path,
    const char* output_cpp_file_path) {
  string output_cpp_file_path_str = string(output_cpp_file_path);

  size_t found;
  found = output_cpp_file_path_str.find_last_of("/");
  const char* vts_name = output_cpp_file_path_str.substr(
      found + 1, output_cpp_file_path_str.length() - found - 5).c_str();

  cout << "vts_name: " << vts_name << endl;

  InterfaceSpecificationMessage message;
  if (InterfaceSpecificationParser::parse(input_vts_file_path, &message)) {
    cout << message.component_class();
  } else {
    cerr << "can't parse " << input_vts_file_path << endl;
  }

  std::stringstream cpp_ss;
  std::stringstream h_ss;

  cpp_ss << "#include \"" << string(input_vts_file_path) << ".h\"" << endl;

  cpp_ss << "#include <iostream>" << endl;
  cpp_ss << "#include \"vts_datatype.h\"" << endl;
  for (auto const& header : message.header()) {
    cpp_ss << "#include " << header << endl;
  }
  cpp_ss << "namespace android {" << endl;
  cpp_ss << "namespace vts {" << endl;

  string component_name = message.original_data_structure_name();
  while (!component_name.empty()
         && (std::isspace(component_name.back())
             || component_name.back() == '*' )) {
    component_name.pop_back();
  }
  const auto pos = component_name.find_last_of(" ");
  if (pos != std::string::npos) {
    component_name = component_name.substr(pos + 1);
  }

  string fuzzer_extended_class_name;
  if (message.component_class() == HAL
      || message.component_class() == HAL_SUBMODULE) {
    fuzzer_extended_class_name = "FuzzerExtended_" + component_name;
  } else {
    cerr << "not yet supported component_class " << message.component_class();
    exit(-1);
  }

  h_ss << "#ifndef __VTS_SPEC_" << vts_name << "__" << endl;
  h_ss << "#define __VTS_SPEC_" << vts_name << "__" << endl;
  h_ss << endl;
  h_ss << "#define LOG_TAG \"" << fuzzer_extended_class_name << "\"" << endl;
  h_ss << "#include <utils/Log.h>" << endl;
  h_ss << "#include \"common/fuzz_tester/FuzzerBase.h\"" << endl;
  for (auto const& header : message.header()) {
    h_ss << "#include " << header << endl;
  }
  h_ss << "\n\n" << endl;
  h_ss << "namespace android {" << endl;
  h_ss << "namespace vts {" << endl;
  h_ss << "class " << fuzzer_extended_class_name << " : public FuzzerBase {"
      << endl;
  h_ss << " protected:" << endl;
  h_ss << "  bool Fuzz(const FunctionSpecificationMessage& func_msg);" << endl;
  if (message.component_class() == HAL_SUBMODULE) {
    // hal_submodule_codegen.GenerateClassHeaderBody();
    h_ss << "  void SetSubModule(" << component_name << "* submodule) {" << endl;
    h_ss << "    submodule_ = submodule;" << endl;
    h_ss << "  }" << endl;
    h_ss << endl;
    h_ss << " private:" << endl;
    h_ss << "  " << message.original_data_structure_name() << "* submodule_;"
        << endl;
  }
  h_ss << "};" << endl;

  string function_name_prefix = GetFunctionNamePrefix(message);

  cpp_ss << endl;
  if (message.component_class() == HAL) {
    cpp_ss << "bool " << fuzzer_extended_class_name << "::Fuzz(" << endl;
    cpp_ss << "    const FunctionSpecificationMessage& func_msg) {" << endl;
    cpp_ss << "  const char* func_name = func_msg.name().c_str();" << endl;
    cpp_ss << "  cout << \"Function: \" << func_name << endl;" << endl;

    for (auto const& api : message.api()) {
      std::stringstream ss;

      cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;

      // args - definition;
      int arg_count = 0;
      for (auto const& arg : api.arg()) {
        cpp_ss << "    " << GetCppVariableType(arg) << " ";
        cpp_ss << "arg" << arg_count << " = ";
        if (arg_count == 0
            && arg.has_aggregate_type()
            && !strncmp(arg.aggregate_type().c_str(),
                        message.original_data_structure_name().c_str(),
                        message.original_data_structure_name().length())) {
          cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << ">(device_)";
        } else {
          cpp_ss << GetCppInstanceType(arg);
        }
        cpp_ss << ";" << endl;
        arg_count++;
      }

      // actual function call
      cpp_ss << "    ";
      if (api.return_type().has_primitive_type()
          && strcmp(api.return_type().primitive_type().c_str(), "void")) {
        cpp_ss << "cout << \"result = \" << ";
      } else if (api.return_type().has_aggregate_type()){
        cpp_ss << "const " << api.return_type().aggregate_type() << " ret1 = ";
      }
      cpp_ss << "reinterpret_cast<" << message.original_data_structure_name()
          << "*>(device_)->" << api.name() << "(";
      if (arg_count > 0) cpp_ss << endl;

      for (int index = 0; index < arg_count; index++) {
        cpp_ss << "      arg" << index;
        if (index != (arg_count - 1)) {
          cpp_ss << "," << endl;
        }
      }
      cpp_ss << ");" << endl;

      if (api.return_type().has_aggregate_type()) {
        /* handle the ret1 */
      }
      cpp_ss << "    return true;" << endl;
      cpp_ss << "  }" << endl;
    }
    // TODO: if there were pointers, free them.
    cpp_ss << "  return false;" << endl;
    cpp_ss << "}" << endl;
  } else if (message.component_class() == HAL_SUBMODULE) {
    cpp_ss << "bool " << fuzzer_extended_class_name << "::Fuzz(" << endl;
    cpp_ss << "    const FunctionSpecificationMessage& func_msg) {" << endl;
    cpp_ss << "  const char* func_name = func_msg.name().c_str();" << endl;
    cpp_ss << "  cout << \"Function: \" << func_name << endl;" << endl;

    for (auto const& api : message.api()) {
      std::stringstream ss;

      cpp_ss << "  if (!strcmp(func_name, \"" << api.name() << "\")) {" << endl;

      // args - definition;
      int arg_count = 0;
      for (auto const& arg : api.arg()) {
        cpp_ss << "    " << GetCppVariableType(arg) << " ";
        cpp_ss << "arg" << arg_count << " = ";
        if (arg_count == 0
            && arg.has_aggregate_type()
            && !strncmp(arg.aggregate_type().c_str(),
                        message.original_data_structure_name().c_str(),
                        message.original_data_structure_name().length())) {
          cpp_ss << "reinterpret_cast<" << GetCppVariableType(arg) << "*>(submodule_)";
        } else {
          cpp_ss << GetCppInstanceType(arg);
        }
        cpp_ss << ";" << endl;
        arg_count++;
      }

      // actual function call
      cpp_ss << "    ";
      if (api.return_type().has_primitive_type()
          && strcmp(api.return_type().primitive_type().c_str(), "void")) {
        cpp_ss << "cout << \"result = \" << ";
      } else if (api.return_type().has_aggregate_type()){
        cpp_ss << "const " << api.return_type().aggregate_type() << " ret1 = ";
      }
      cpp_ss << "reinterpret_cast<" << message.original_data_structure_name()
          << "*>(submodule_)->" << api.name() << "(";
      if (arg_count > 0) cpp_ss << endl;

      for (int index = 0; index < arg_count; index++) {
        cpp_ss << "      arg" << index;
        if (index != (arg_count - 1)) {
          cpp_ss << "," << endl;
        }
      }
      cpp_ss << ");" << endl;

      if (api.return_type().has_aggregate_type()){
        // TODO: handle ret1
      }
      cpp_ss << "    return true;" << endl;
      cpp_ss << "  }" << endl;
    }
    // TODO: if there were pointers, free them.
    cpp_ss << "  return false;" << endl;
    cpp_ss << "}" << endl;
  }

  std::stringstream ss;

  // return type
  h_ss << endl;
  ss << "android::vts::FuzzerBase* " << endl;

  // function name
  ss << function_name_prefix << "(" << endl;
  ss << ")";

  if (message.component_class() == HAL) {
    h_ss << "extern \"C\" {" << endl;
    h_ss << "extern " << ss.str() << ";" << endl;
    h_ss << "}" << endl;
  }

  cpp_ss << "}  // namespace vts" << endl;
  cpp_ss << "}  // namespace android" << endl;
  cpp_ss << endl << endl;

  if (message.component_class() == HAL) {
    cpp_ss << "extern \"C\" {" << endl;
    cpp_ss << ss.str() << " {" << endl;
    cpp_ss << "  return (android::vts::FuzzerBase*) "
        << "new android::vts::" << fuzzer_extended_class_name << "();" << endl;
    cpp_ss << "}" << endl << endl;
    cpp_ss << "}" << endl;
  }

  h_ss << "}  // namespace vts" << endl;
  h_ss << "}  // namespace android" << endl;
  h_ss << "#endif" << endl;

  cout << "write to " << output_cpp_file_path << endl;
  ofstream cpp_out_file(output_cpp_file_path);
  if (!cpp_out_file.is_open()) {
    cerr << "Unable to open file" << endl;
  } else {
    cpp_out_file << cpp_ss.str();
    cpp_out_file.close();
  }

  string output_header_file_path = string(output_header_dir_path)
      + "/" + string(input_vts_file_path);

  output_header_file_path = output_header_file_path + ".h";

  cout << "header: " << output_header_file_path << endl;
  vts_fs_mkdirs(output_header_file_path.c_str(), 0777);

  ofstream header_out_file(output_header_file_path.c_str());
  if (!header_out_file.is_open()) {
    cerr << "Unable to open file" << endl;
  } else {
    header_out_file << h_ss.str();
    header_out_file.close();
  }
}

}  // namespace vts
}  // namespace android


int main(int argc, char* argv[]) {
  cout << "Android VTS Compiler (AVTSC)" << endl;
  for (int i = 0; i < argc; i++) {
    cout << "- args[" << i << "] " << argv[i] << endl;
  }
  if (argc < 5) {
    cerr << "argc " << argc << " < 5" << endl;
    return -1;
  }
  android::vts::Translate(argv[2], argv[3], argv[4]);
  return 0;
}
