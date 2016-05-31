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

#include "VtsCompilerUtils.h"
#include "code_gen/CodeGenBase.h"
#include "code_gen/HalCodeGen.h"
#include "code_gen/HalSubmoduleCodeGen.h"
#include "code_gen/LegacyHalCodeGen.h"


using namespace std;


namespace android {
namespace vts {


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

  unique_ptr<CodeGenBase> code_generator;
  switch (message.component_class()) {
    case HAL:
      code_generator.reset(new HalCodeGen(input_vts_file_path, vts_name));
      break;
    case HAL_SUBMODULE:
      code_generator.reset(
          new HalSubmoduleCodeGen(input_vts_file_path, vts_name));
      break;
    case LEGACY_HAL:
      code_generator.reset(new LegacyHalCodeGen(input_vts_file_path, vts_name));
      break;
    default:
      cerr << "not yet supported component_class " << message.component_class();
      exit(-1);
  }

  std::stringstream cpp_ss;
  std::stringstream h_ss;
  code_generator->GenerateAll(cpp_ss, h_ss, message);

  // Creates a C/C++ file and its header file.
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
  vts_fs_mkdirs(&output_header_file_path[0], 0777);

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
