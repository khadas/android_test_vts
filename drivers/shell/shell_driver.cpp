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
#include "shell_msg_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>


/**
 * struct to save status of reading shell command output
 */
typedef struct output_struct {
  bool  is_fully_read;
  FILE* output_fp;
  char  buffer[4096];
} output_struct;


/**
 * read output once for the given length and save status to the struct
 */
static bool read_output(output_struct* output) {
  if (output == NULL) {
    fprintf(stderr, "error: NULL reference of output\n");
    return false;
  }

  fread(output->buffer, sizeof(output->buffer), 1, output->output_fp);
  output->is_fully_read = feof(output->output_fp);

  return !ferror(output->output_fp);
}

/**
 * close file path for output and free memory
 */
static int close_output(output_struct* output) {
  // TODO(yuexima): call close on output;
  int close_success;
  close_success = pclose(output -> output_fp);
  free(output);

  return close_success;
}


/*
 * execute a given shell command and return the output file descriptor
 * Please remember to call close_output after usage.
 */
static output_struct* exec_shell_cmd(char* cmd) {
  // TODO(yuexima): handle no output case.
  FILE* output_fp;

  // execute the command.
  output_fp = popen(cmd, "r");
  if (output_fp == NULL) {
    fprintf(stderr, "Failed to run command\n");
    exit(errno);
  }

  output_struct* output = (output_struct*)malloc(sizeof(output_struct));
  memset((void*)output, 0, sizeof(output));

  output->output_fp = output_fp;

  return read_output(output) ? output : NULL;
}


/*
 * Handles a socket connection. Will execute a received shell command
 * and send back the output text.
 */
static int connection_handler_shell_cmd(int connection_fd) {
  // TODO(yuexima): handle multiple commands in a while loop

  char* cmd;
  cmd = read_with_length(connection_fd);

  printf("Driver: received command [\"%s\"]. Processing... \n", cmd);

  // execute command and write back output
  output_struct* output = exec_shell_cmd(cmd);

  bool read_success;

  // TODO(yuexima): check success
  do {
    write_with_length(connection_fd, output->buffer);
    printf("Driver: wrote output: [%s]\n", output->buffer);
    read_success = read_output(output);
    if (!read_success) {
      fprintf(stderr, "Driver: read error %d\n", read_success);
      break;
    }
  } while (!output->is_fully_read);

  printf("Driver: finished processing command [\"%s\"].\n", cmd);

  int close_conn_success;
  int close_output_success;

  free(cmd);
  close_conn_success = close(connection_fd);
  if (close_conn_success != 0) {
    fprintf(stderr, "Driver: failed to close connection (errno: %d).\n", errno);
  }
  close_output_success = close_output(output);
  if (close_output_success != 0) {
    fprintf(stderr,
            "Driver: failed to close output buffer. (pclose: %d).\n",
            close_output_success);
  }

  return read_success && close_conn_success == 0 && close_output_success == 0 ? 0 : 1;
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

  if (bind(socket_fd,
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
                           (struct sockaddr *)&address, &address_length);
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


