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


#include "shell_driver_test.h"

#include <gtest/gtest.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "shell_driver_test_client.h"
// include code to be tested
#include "shell_driver.h"

using namespace std;

namespace android {
namespace vts {


/*
 * Prototype unit test helper. It first forks a vts_shell_driver process
 * and then call a client function to execute a command.
 */
static char* test_shell_command_output(char* command, char* addr_socket) {
  int res = 0;
  pid_t p_driver;
  char* res_client;

  p_driver = fork();
  if (p_driver == 0) {  // child
    int res_driver = vts_shell_driver_start(addr_socket);

    if (res_driver != 0) {
      fprintf(stderr,
              "Driver reported error. The error code is: %d.\n", res_driver);
      exit(res_driver);
    }

    exit(0);
  } else if (p_driver > 0) {  // parent
    res_client = vts_shell_driver_test_client_start(command, addr_socket);

    int res_client_len;
    res_client_len = strlen(res_client);
    if (res_client == NULL) {
      fprintf(stderr, "Client reported error.\n");
      exit(1);
    }
    cout << "Client receiving: " << res_client << endl;
  } else {
    fprintf(stderr,
            "shell_driver_test.cpp: create child process failed for driver.");
    exit(-1);
  }

  // send kill signal to insure the process would not block
  kill(p_driver, SIGKILL);

  return res_client;
}


/*
 * This test tests whether the output of "uname" is "Linux\n"
 */
TEST(vts_shell_driver_start, vts_shell_driver_unit_test_uname) {
  char cmd[] = "uname";
  char expected[] = "Linux\n";
  char addr_socket[] = "test1_1.tmp";
  char* output;

  output = test_shell_command_output(cmd, addr_socket);
  ASSERT_TRUE(!strcmp(output, expected));

  free(output);
}

/*
 * This test tests whether the output of "uname" is "Linux\n"
 */
TEST(vts_shell_driver_start, vts_shell_driver_unit_test_which_ls) {
  char cmd[] = "which ls";
  char expected[] = "/system/bin/ls\n";
  char addr_socket[] = "test1_2.tmp";
  char* output;

  output = test_shell_command_output(cmd, addr_socket);
  ASSERT_TRUE(!strcmp(output, expected));

  free(output);
}

}  // namespace vts
}  // namespace android
