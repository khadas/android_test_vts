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

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>
#include <sstream>

using namespace std;

namespace android {
namespace vts {

bool VtsSocketSend(int sockfd, const string& message) {
  std::stringstream header;
  header << message.length() << "\n";
  cout << "[agent->driver] len = " << message.length() << endl;
  int n = write(sockfd, header.str().c_str(), header.str().length());
  if (n < 0) {
    cerr << __func__ << ":" << __LINE__ << " ERROR writing to socket" << endl;
    return false;
  }

  n = write(sockfd, message.c_str(), message.length());
  if (n < 0) {
    cerr << __func__ << ":" << __LINE__ << " ERROR writing to socket" << endl;
    return false;
  }
  cout << "[agent->driver] sent" << endl;
  return true;
}

#define MAX_HEADER_BUFFER_SIZE 128


string VtsSocketRecv(int sockfd) {
  int header_index = 0;
  char header_buffer[MAX_HEADER_BUFFER_SIZE];

  for (header_index = 0; header_index < MAX_HEADER_BUFFER_SIZE; header_index++) {
    if (read(sockfd, &header_buffer[header_index], 1) != 1) {
      cerr << __func__ << " ERROR reading the length" << endl;
      return string();
    }
    if (header_buffer[header_index] == '\n'
        || header_buffer[header_index] == '\r') {
      header_buffer[header_index] = '\0';
      break;
    }
  }

  int msg_len = atoi(header_buffer);
  char* msg = (char*) malloc(msg_len + 1);
  if (!msg) {
    cerr << __func__ << " ERROR malloc failed" << endl;
    return string();
  }

  if (read(sockfd, msg, msg_len) !=  msg_len) {
    cerr << __func__ << " ERROR read failed" << endl;
    return string();
  }
  msg[msg_len] = '\0';
  return string(msg, msg_len);
}

}  // namespace vts
}  // namespace android
