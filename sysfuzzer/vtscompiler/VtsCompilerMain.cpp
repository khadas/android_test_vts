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

int main(int argc, char* argv[]) {
#ifdef VTS_DEBUG
  cout << "Android VTS Compiler (AVTSC)" << endl;
#endif
  int opt_count = 0;
  android::vts::VtsCompileMode mode = android::vts::kDriver;
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
    }
  }
  if (argc < 5) {
    cerr << "argc " << argc << " < 5" << endl;
    return -1;
  }
  android::vts::Translate(
      mode, argv[opt_count + 1], argv[opt_count + 2], argv[opt_count + 3]);
  return 0;
}
