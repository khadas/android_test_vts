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

#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "code_gen/CodeGenBase.h"

using namespace std;

// To generate both header and source files,
//   Usage: vtsc -mDRIVER | -mPROFILER <.vts input file path> \
//          <header output dir> <C/C++ source output file path>
// To generate only a header file,
//   Usage: vtsc -mDRIVER | -mPROFILER -tHEADER <.vts input file path> \
//          <header output file path>
// To generate only a source file,
//   Usage: vtsc -mDRIVER | -mPROFILER -tSOURCE <.vts input file path> \
//          <C/C++ source output file path>

int main(int argc, char* argv[]) {
#ifdef VTS_DEBUG
  cout << "Android VTS Compiler (AVTSC)" << endl;
#endif
  int opt_count = 0;
  android::vts::VtsCompileMode mode = android::vts::kDriver;
  android::vts::VtsCompileFileType type = android::vts::VtsCompileFileType::kBoth;
  for (int i = 0; i < argc; i++) {
#ifdef VTS_DEBUG
    cout << "- args[" << i << "] " << argv[i] << endl;
#endif
    if (argv[i] && strlen(argv[i]) > 1 && argv[i][0] == '-') {
      opt_count++;
      if (argv[i][1] == 'm') {
        if (!strcmp(&argv[i][2], "PROFILER")) {
          mode = android::vts::kProfiler;
#ifdef VTS_DEBUG
          cout << "- mode: PROFILER" << endl;
#endif
        }
      }
      if (argv[i][1] == 't') {
        if (!strcmp(&argv[i][2], "HEADER")) {
          type = android::vts::kHeader;
#ifdef VTS_DEBUG
          cout << "- type: HEADER" << endl;
#endif
        } else if (!strcmp(&argv[i][2], "SOURCE")) {
          type = android::vts::kSource;
#ifdef VTS_DEBUG
          cout << "- type: SOURCE" << endl;
#endif
        }
      }
    }
  }
  if (argc < 5) {
    cerr << "argc " << argc << " < 5" << endl;
    return -1;
  }
  switch (type) {
    case android::vts::kBoth:
      android::vts::Translate(
          mode, argv[opt_count + 1], argv[opt_count + 2], argv[opt_count + 3]);
      break;
    case android::vts::kHeader:
    case android::vts::kSource:
      android::vts::TranslateToFile(
          mode, argv[opt_count + 1], argv[opt_count + 2], type);
      break;
  }
  return 0;
}
