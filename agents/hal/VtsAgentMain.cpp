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

#include <unistd.h>

#include <iostream>

#include "TcpServerForRunner.h"

#define DEFAULT_FUZZER_FILE_PATH32 "./fuzzer32"
#define DEFAULT_FUZZER_FILE_PATH64 "./fuzzer64"


int main(int argc, char* argv[]) {
  char* fuzzer_path32;
  char* fuzzer_path64;
  char* spec_dir_path = NULL;

  if (argc == 1) {
      fuzzer_path32 = DEFAULT_FUZZER_FILE_PATH32;
      fuzzer_path64 = DEFAULT_FUZZER_FILE_PATH64;
  } else if (argc == 2) {
      fuzzer_path32 = DEFAULT_FUZZER_FILE_PATH32;
      fuzzer_path64 = DEFAULT_FUZZER_FILE_PATH64;
      spec_dir_path = argv[1];
  } else if (argc == 3) {
      fuzzer_path32 = argv[1];
      fuzzer_path64 = argv[2];
  } else if (argc == 4) {
      fuzzer_path32 = argv[1];
      fuzzer_path64 = argv[2];
      spec_dir_path = argv[3];
  } else {
      std::cerr << "usage: vts_hal_agent "
          << "[[<fuzzer 32-bit binary path> [<fuzzer 64-bit binary path>] "
          << "[<spec file base dir path>]]" << std::endl;
      return -1;
  }

  char* dir_path;
  dir_path = (char*) malloc(strlen(argv[0]) + 1);
  strcpy(dir_path, argv[0]);
  for (int index = strlen(argv[0]) - 2; index >= 0; index--) {
    if (dir_path[index] == '/') {
      dir_path[index] = '\0';
      break;
    }
  }
  printf("chdir %s\n", dir_path);
  chdir(dir_path);

  android::vts::StartTcpServerForRunner(
      (const char*) fuzzer_path32, (const char*) fuzzer_path64,
      (const char*) spec_dir_path);
  return 0;
}
