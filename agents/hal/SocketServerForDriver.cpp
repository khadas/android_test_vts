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

#include "test/vts/runners/host/proto/AndroidSystemControlMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

const static int kCallbackServerPort = 5010;


void RpcCallToRunner(char* id, int runner_port) {
  cout << __func__ << ":" << __LINE__ << " " << id << endl;
  struct sockaddr_in serv_addr;
  struct hostent* server;

  for (int index = 0; index < strlen(id); index++) {
    if (id[index] == '\n' || id[index] == '\r') {
      id[index] = '\0';
      break;
    }
  }

  int sockfd;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    cerr << __func__ << " ERROR opening socket" << endl;
    exit(-1);
    return;
  }
  server = gethostbyname("127.0.0.1");
  if (server == NULL) {
    cerr << __func__ << " ERROR can't resolve the host name, localhost" << endl;
    exit(-1);
    return;
  }
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(runner_port);

  if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    cerr << __func__ << " ERROR connecting" << endl;
    exit(-1);
    return;
  }

  cout << "sending " << id << endl;
  AndroidSystemCallbackRequestMessage* callback_msg =
      new AndroidSystemCallbackRequestMessage();
  callback_msg->set_id(id);

  string callback_msg_str;
  if (!callback_msg->SerializeToString(&callback_msg_str)) {
    cerr << "can't serialize the callback request message to a string." << endl;
    return;
  }

  std::stringstream callback_header;
  callback_header << callback_msg_str.length() << "\n";
  int n = write(sockfd, callback_header.str().c_str(),
                callback_header.str().length());
  if (n < 0) {
    cerr << __func__ << ":" << __LINE__ << " ERROR writing to socket" << endl;
    exit(-1);
    return;
  }

  n = write(sockfd, callback_msg_str.c_str(), callback_msg_str.length());
  if (n < 0) {
    cerr << __func__ << ":" << __LINE__ << " ERROR writing to socket" << endl;
    exit(-1);
    return;
  }
  delete callback_msg;
  close(sockfd);
  return;
}


void handler(int sock, int runner_port) {
  int n;
  char buffer[256];

  bzero(buffer, 256);
  n = read(sock, buffer, 255);
  if (n < 0) {
    cerr << __func__ << " ERROR reading from socket" << endl;
    return;
  }
  cout << "Here is the message: " << buffer << endl;
  RpcCallToRunner(buffer, kCallbackServerPort);
}


int StartSocketServerForDriver(int agent_port, int runner_port) {
  struct sockaddr_in serv_addr;

  int pid;
  pid = fork();
  if (pid < 0) {
    cerr << __func__ << " ERROR on fork" << endl;
    return -1;
  } else if (pid > 0) {
    return 0;
  }

  // only child process continues;
  int sockfd;
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    cerr << __func__ << " ERROR opening socket" << endl;
    return -1;
  }
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(agent_port);

  if (::bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    cerr << __func__ << " ERROR on binding" << endl;
    return -1;
  }

  if (listen(sockfd, 5) < 0) {
    cerr << __func__ << " ERROR on listening" << endl;
    return -1;
  }

  while (1) {
    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen;

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
    if (newsockfd < 0) {
      cerr << __func__ << " ERROR on accept " << strerror(errno) << endl;
      break;
    }
    pid = fork();
    if (pid < 0) {
      cerr << __func__ << " ERROR on fork" << endl;
      break;
    }
    if (pid == 0)  {
      // close(sockfd);
      handler(newsockfd, runner_port);
      exit(0);
    } else {
      //close(newsockfd);
    }
  }
  close(sockfd);
  exit(0);
}

}  // namespace vts
}  // namespace android
