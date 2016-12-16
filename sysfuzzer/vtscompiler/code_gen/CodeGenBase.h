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

#ifndef __VTS_SYSFUZZER_COMPILER_CODEGENBASE_H__
#define __VTS_SYSFUZZER_COMPILER_CODEGENBASE_H__

#include <hidl-util/Formatter.h>
#include <iostream>
#include <sstream>
#include <string>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

enum VtsCompileMode {
  kDriver = 0,
  kProfiler
};

class CodeGenBase {
 public:
  explicit CodeGenBase(const char* input_vts_file_path, const string& vts_name);
  virtual ~CodeGenBase();

  // Generate both a C/C++ file and its header file.
  virtual void GenerateAll(Formatter& header_out, Formatter& source_out,
                           const ComponentSpecificationMessage& message) = 0;

  const char* input_vts_file_path() const {
    return input_vts_file_path_;
  }

  const string& vts_name() const {
    return vts_name_;
  }

 protected:
  const char* input_vts_file_path_;
  const string& vts_name_;
};

void Translate(VtsCompileMode mode,
               const char* input_vts_file_path,
               const char* output_header_dir_path,
               const char* output_cpp_file_path);

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMPILER_CODEGENBASE_H__
