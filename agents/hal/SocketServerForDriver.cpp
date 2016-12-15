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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>

using namespace std;

namespace android {
namespace vts {


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
  // TODO: forward the call to the runner's server TCP port.
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
