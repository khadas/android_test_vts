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


#include "shell_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <iostream>

#include "shell_msg_protocol.h"
#include "test/vts/proto/VtsDriverControlMessage.pb.h"

using namespace std;

namespace android {
namespace vts {


/*
 * execute a given shell command and return the output file descriptor
 * Please remember to call close_output after usage.
 */
static int exec_shell_cmd(string cmd,
                          VtsDriverControlResponseMessage* out_msg) {
  // TODO(yuexima): handle no output case.
  FILE* output_fp;

  // execute the command.
  output_fp = popen(cmd.c_str(), "r");
  if (output_fp == NULL) {
    fprintf(stderr, "Failed to run command\n");
    int no = errno;
    return no;
  }

  char buf[4096];

  stringstream ss;

  while (!feof(output_fp)) {
    fread(buf, sizeof(buf), 1, output_fp);

    // TODO(yuexima) catch stderr
    if (ferror(output_fp)) {
      fprintf(stderr, "error: output read error\n");
      return -1;
    }

    string buf_str(buf);
    ss << buf_str;
  }

  out_msg->add_stdout(ss.str());

  return 0;
}


/*
 * Handles a socket connection. Will execute a received shell command
 * and send back the output text.
 */
static int connection_handler_shell_cmd(int connection_fd) {
  // TODO(yuexima): handle multiple commands in a while loop
  VtsDriverControlCommandMessage cmd_msg;

  int success;
  int nfailure = 0;

  success = read_pb_msg(connection_fd, (google::protobuf::Message*) &cmd_msg);
  if (success != 0) {
    fprintf(stderr, "Driver: read command error.\n");
    return -1;
  }

  cout << "[Shell driver] received " << cmd_msg.shell_command_size()
      << " command(s). Processing... " << endl;

  // execute command and write back output
  VtsDriverControlResponseMessage out_msg;

  for (const auto& command : cmd_msg.shell_command()) {
    success = exec_shell_cmd(command, &out_msg);
    if (success != 0) {
      cerr << "[Shell driver] error during executing command ["
          << command << "]" << endl;
      nfailure--;
    }
  }

  success = write_pb_msg(connection_fd,
                         (google::protobuf::Message*) &out_msg);
  if (success != 0) {
    fprintf(stderr, "Driver: write output to socket error.\n");
    nfailure--;
  }

  cout << "[Shell driver] finished processing commands." << endl;

  success = close(connection_fd);
  if (success != 0) {
    fprintf(stderr, "Driver: failed to close connection (errno: %d).\n", errno);
    nfailure--;
  }

  return nfailure;
}


int vts_shell_driver_start(char* addr_socket) {
  struct sockaddr_un address;
  int socket_fd, connection_fd;
  socklen_t address_length;
  pid_t child;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    fprintf(stderr, "socket() failed\n");
    return socket_fd;
  }

  unlink(addr_socket);
  memset(&address, 0, sizeof(struct sockaddr_un));
  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, addr_socket);

  if (::bind(socket_fd,
             (struct sockaddr *) &address,
             sizeof(struct sockaddr_un)) != 0) {
    fprintf(stderr, "bind() failed: errno = %d\n", errno);
    return 1;
  }

  if (listen(socket_fd, 5) != 0) {
    fprintf(stderr, "Driver: listen() failed: errno = %d\n", errno);

    return errno;
  }

  while (1) {
    address_length = sizeof(address);

    connection_fd = accept(socket_fd,
                           (struct sockaddr *) &address, &address_length);
    if (connection_fd == -1) {
      fprintf(stderr, "accept error: %s\n", strerror(errno));
      break;
    }

    child = fork();
    if (child == 0) {
      // now inside newly created connection handling process
      int res = connection_handler_shell_cmd(connection_fd);
      if (res == 0) {  // success
        return 0;
      } else {
        fprintf(stderr, "Driver: connection handler returned failure");
        return 1;
      }
    } else if (child > 0) {
      // parent - no op.
    } else {
      fprintf(stderr,
              "shell_driver.c: create child process failed. Exiting...");
      return(errno);
    }

    close(connection_fd);
  }

  // clean up
  close(socket_fd);
  unlink(addr_socket);
  free(addr_socket);

  return 0;
}

}  // namespace vts
}  // namespace android

