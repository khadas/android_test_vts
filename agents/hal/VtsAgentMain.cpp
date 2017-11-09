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

#include <getopt.h>
#include <map>

#include <android-base/logging.h>
#include "TcpServerForRunner.h"

#define DEFAULT_HAL_DRIVER_FILE_PATH32 "./vts_hal_driver32"
#define DEFAULT_HAL_DRIVER_FILE_PATH64 "./vts_hal_driver64"
#define DEFAULT_SHELL_DRIVER_FILE_PATH32 "./vts_shell_driver32"
#define DEFAULT_SHELL_DRIVER_FILE_PATH64 "./vts_shell_driver64"

using namespace std;

int main(int argc, char* argv[]) {
  android::base::InitLogging(argv, android::base::StderrLogger);

  char* spec_dir_path = NULL;
  char* hal_path32;
  char* hal_path64;
  char* shell_path32;
  char* shell_path64;

  LOG(INFO) << "|| VTS AGENT ||";

  if (argc == 1) {
    hal_path32 = (char*) DEFAULT_HAL_DRIVER_FILE_PATH32;
    hal_path64 = (char*) DEFAULT_HAL_DRIVER_FILE_PATH64;
    shell_path32 = (char*) DEFAULT_SHELL_DRIVER_FILE_PATH32;
    shell_path64 = (char*) DEFAULT_SHELL_DRIVER_FILE_PATH64;
  } else if (argc == 2) {
    hal_path32 = (char*) DEFAULT_HAL_DRIVER_FILE_PATH32;
    hal_path64 = (char*) DEFAULT_HAL_DRIVER_FILE_PATH64;
    spec_dir_path = argv[1];
    shell_path32 = (char*) DEFAULT_SHELL_DRIVER_FILE_PATH32;
    shell_path64 = (char*) DEFAULT_SHELL_DRIVER_FILE_PATH64;
  } else if (argc == 3) {
    hal_path32 = argv[1];
    hal_path64 = argv[2];
  } else if (argc == 4) {
    hal_path32 = argv[1];
    hal_path64 = argv[2];
    spec_dir_path = argv[3];
  } else if (argc == 6) {
    hal_path32 = argv[1];
    hal_path64 = argv[2];
    spec_dir_path = argv[3];
    shell_path32 = argv[4];
    shell_path64 = argv[5];
  } else {
    LOG(ERROR) << "usage: vts_hal_agent "
               << "[[<hal 32-bit binary path> [<hal 64-bit binary path>] "
               << "[<spec file base dir path>]]"
               << "[[<shell 32-bit binary path> [<shell 64-bit binary path>] ";
    return -1;
  }

  char* dir_path;
  dir_path = (char*)malloc(strlen(argv[0]) + 1);
  strcpy(dir_path, argv[0]);
  for (int index = strlen(argv[0]) - 2; index >= 0; index--) {
    if (dir_path[index] == '/') {
      dir_path[index] = '\0';
      break;
    }
  }
  chdir(dir_path);

  android::vts::StartTcpServerForRunner(
      (const char*)spec_dir_path, (const char*)hal_path32,
      (const char*)hal_path64, (const char*)shell_path32,
      (const char*)shell_path64);
  return 0;
}
