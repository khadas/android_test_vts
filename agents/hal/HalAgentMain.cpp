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

#include <iostream>

#include "TcpServer.h"

#define DEFAULT_FUZZER_FILE_PATH "./fuzzer"


int main(int argc, char* argv[]) {
  char* fuzzer_path;
  char* spec_dir_path = NULL;

  if (argc == 1) {
      fuzzer_path = DEFAULT_FUZZER_FILE_PATH;
  } else if (argc == 2) {
    fuzzer_path = argv[1];
  } else if (argc == 3) {
    fuzzer_path = argv[1];
    spec_dir_path = argv[2];
  } else {
    std::cerr << "usage: vts_hal_agent "
        << "[<fuzzer binary path> [<spec file base dir path>]]" << std::endl;
    return -1;
  }
  android::vts::StartTcpServer(
      (const char*) fuzzer_path, (const char*) spec_dir_path);
  return 0;
}
