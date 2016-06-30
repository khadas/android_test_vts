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

#include <string>
#include <iostream>
#include <sstream>

#include "test/vts/proto/VtsDriverControlMessage.pb.h"

using namespace std;

namespace android {
namespace vts {


int kLengthTextBufferSize = 6;
char kAddressUnixSocketPrefix[] = "./tmp_unix_socket_test/";


/**
 * Read one line from socket.
 * Reading stops when new line or tab char is read into the buffer.
 */
static int readline(char* buf, int size, int fd) {
  int idx = 0;

  while (idx < size
         && read(fd, &buf[idx], 1) == 1) {
    if (buf[idx] == '\n' || buf[idx] == '\t') break;
    idx++;
  }

  return idx;
}


int read_len_header(int fd) {
  char msg_buf[kLengthTextBufferSize];

  int len_read;

  len_read = readline(msg_buf, sizeof(msg_buf), fd);

  if (len_read < 0) {
    fprintf(stderr, "read error.\n");
    return len_read;
  }

  return stoi(msg_buf);
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


char* read_with_length(int fd) {
  // reads the length of shell command
  // our protocol specifies a new line char to be the separator.

  int msglen = read_len_header(fd);

  if (msglen < 0) {
    fprintf(stderr, "error in reading message length header\n");
  }


  // read command from client
  char* msg = (char*) malloc(msglen + 1);

  int nbytes;
  nbytes = read(fd, msg, msglen);

  if (nbytes < 0) {
    // check connection read success
    fprintf(stderr, "read connection failed.");

    exit(errno);
  } else if (nbytes != msglen) {
    // check whether the length read is expected
    fprintf(stderr, "read command from client with unexpected length.\n");

    exit(1);
  }

  // set the end of string
  msg[nbytes] = 0;

  return msg;
}


int write_pb_msg(int fd, google::protobuf::Message* msg) {
  if (!msg) {
    cerr << "msg == NULL" << endl;
    return -1;
  }

  string msg_str;
  if (!msg->SerializeToString(&msg_str)) {
    cerr << "can't serialize the message to a string." << endl;
    return -1;
  }

  stringstream header;
  header << msg_str.length() << "\n";

  int n;
  n = write(fd, header.str().c_str(), header.str().length());
  if (n < 0) {
    cerr << "ERROR writing to socket" << endl;
    return -1;
  }

  cout << "[Shell driver] sending " << msg_str.length() << " bytes" << endl;

  n = write(fd, msg_str.c_str(), msg_str.length());
  if (n < 0) {
    cerr << "[Shell driver] ERROR writing to socket" << endl;
    return -1;
  }

  return 0;
}


int read_pb_msg(int fd, google::protobuf::Message* msg) {
  if (fd < 0) {
    cerr << "invalid fd: " << fd << endl;
    return -1;
  }

  int data_len = read_len_header(fd);
  if (data_len < 0) {
    cerr << "length header read failure: " << data_len << endl;
    return -1;
  }

  // read command from client
  char* data = (char*) malloc(data_len + 1);

  int n;
  n = read(fd, data, data_len);

  if (n < 0) {  // check connection read success
    cerr << "read connection failed. errno = " << errno << endl;
    return -1;
  } else if (n != data_len) {  // check whether the length read is expected
    cerr << "read command from client with unexpected length." << endl;
    return -1;
  }

  data[data_len] = 0;

  if (!data) {
    cerr << "read data failed." << endl;
    return -1;
  }

  string msg_str(data, data_len);
  free(data);

  bool parse_success;
  parse_success = msg->ParseFromString(msg_str);
  if (!parse_success) {
    cerr << "Shell driver protocol buffer message parse error." << endl;
    return -1;
  }

  return 0;
}

}  // namespace vts
}  // namespace android

