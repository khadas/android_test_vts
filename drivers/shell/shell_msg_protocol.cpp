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

#include "shell_msg_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>


int kLengthTextBufferSize = 6;
char kAddressUnixSocketPrefix[] = "./tmp_unix_socket_test/";

/*
 * read a line from socket connection into a char buffer up to length of size.
 * The length read will be the return value;
 */
static int readline(char* buf, int size, int connection_fd) {
  int idx = 0;

  while (idx < size
         && read(connection_fd, &buf[idx], 1) == 1) {
    if (buf[idx] == '\n') break;
    idx++;
  }

  return idx;
}


int write_with_length(int connection_fd, char* msg) {
  // get the length of the command
  int len = strlen(msg);

  // allocate space for converting length int into text
  char lenstr[kLengthTextBufferSize];

  // print length int into text string
  sprintf(lenstr, "%d\n", len);

  // length of the length string
  int len_lenstr = strlen(lenstr);

  // length actual written to socket
  int len_written;

  // write the length text string to the receiver with a new line char
  len_written = write(connection_fd, lenstr, len_lenstr);
  if (len_written != len_lenstr) {
    fprintf(stderr, "length written to socket is different from expected: ");
    fprintf(stderr, "expected: %d, actual:%d\n", len_lenstr, len_written);

    return 1;
  }

  // write the actual msg string
  len_written = write(connection_fd, msg, len);
  if (len_written != len) {
    fprintf(stderr, "length written to socket is different from expected: ");
    fprintf(stderr, "expected: %d, actual:%d\n", len, len_written);

    return 1;
  }

  return 0;
}


char* read_with_length(int connection_fd) {
  // reads the length of shell command
  // our protocol specifies a new line char to be the separator.
  char msg_buf[kLengthTextBufferSize];

  int len_read;

  len_read = readline(msg_buf, sizeof(msg_buf), connection_fd);

  if (len_read < 0) {
    fprintf(stderr, "read error.\n");
    return NULL;
  }

  int msglen = atoi(msg_buf);

  // read command from client
  char* msg = (char*)malloc(msglen + 1);

  int nbytes;
  nbytes = read(connection_fd, msg, msglen);

  if (nbytes < 0) {
    // check connection read success
    fprintf(stderr, "read connection failed.");

    exit(errno);
  } else if (nbytes != msglen) {
    // check whether the length read is expected
    fprintf(stderr, "read command from client with unexpected length.");

    exit(1);
  }

  // set the end of string
  msg[nbytes] = 0;

  return msg;
}
