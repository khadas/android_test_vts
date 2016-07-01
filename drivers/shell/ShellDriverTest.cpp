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


#include "ShellDriverTest.h"

#include <gtest/gtest.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <sstream>
#include <iostream>

#include "test/vts/proto/VtsDriverControlMessage.pb.h"
#include <VtsDriverCommUtil.h>

#include "ShellDriver.h"

using namespace std;

namespace android {
namespace vts {


static int kMaxRetry = 3;

/*
 * send a command to the driver on specified UNIX domain socket and print out
 * the outputs from driver.
 */
static char* vts_shell_driver_test_client_start(char* cmd, char* addr_socket) {
  struct sockaddr_un address;
  int socket_fd;
  int nbytes;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    fprintf(stderr, "socket() failed\n");
    return NULL;
  }

  VtsDriverCommUtil driverUtil(socket_fd);

  memset(&address, 0, sizeof(struct sockaddr_un));

  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, addr_socket);

  int conn_success;
  int retry_count = 0;

  conn_success = connect(socket_fd, (struct sockaddr*) &address,
                         sizeof(struct sockaddr_un));
  for (retry_count = 0; retry_count < kMaxRetry && conn_success != 0;
      retry_count++) {  // retry if server not ready
    printf("Client: connection failed, retrying...\n");
    retry_count++;
    if (usleep(50 * pow(retry_count, 3)) != 0) {
      fprintf(stderr, "shell driver unit test: sleep intrupted.");
    }

    conn_success = connect(socket_fd, (struct sockaddr*) &address,
                           sizeof(struct sockaddr_un));
  }

  if (conn_success != 0) {
    fprintf(stderr, "connect() failed\n");
    return NULL;
  }

  VtsDriverControlCommandMessage cmd_msg;

  string cmd_str(cmd);
  cmd_msg.add_shell_command(cmd_str);

  if (!driverUtil.VtsSocketSendMessage(cmd_msg)) {
    return NULL;
  }

  // read driver output
  VtsDriverControlResponseMessage out_msg;

  if (!driverUtil.VtsSocketRecvMessage(
          static_cast<google::protobuf::Message*>(&out_msg))) {
    return NULL;
  }


  // TODO(yuexima) use vector for output messages
  stringstream ss;
  for (int i = 0; i < out_msg.stdout_size(); i++) {
    string out_str = out_msg.stdout(i);
    cout << "[Shell driver] output for command " << i
        << ": " << out_str << endl;
    ss << out_str;
  }

  close(socket_fd);

  string res_str = ss.str();
  char* res = static_cast<char*>(malloc(res_str.length() + 1));
  strcpy(res, res_str.c_str());

  cout << "[Client] receiving output: " << res << endl;

  return res;
}


/*
 * Prototype unit test helper. It first forks a vts_shell_driver process
 * and then call a client function to execute a command.
 */
static char* test_shell_command_output(char* command, char* addr_socket) {
  int res = 0;
  pid_t p_driver;
  char* res_client;

  VtsShellDriver shellDriver(addr_socket);

  p_driver = fork();
  if (p_driver == 0) {  // child
    int res_driver = shellDriver.StartListen();

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
