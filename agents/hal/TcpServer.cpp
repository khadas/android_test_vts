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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include <netdb.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <utils/RefBase.h>

#include <iostream>
#include <sstream>

#include "test/vts/agents/hal/proto/AndroidSystemControlMessage.pb.h"
#include "test/vts/sysfuzzer/common/proto/InterfaceSpecificationMessage.pb.h"
#include "BinderClient.h"
#include "RequestHandler.h"

using namespace std;

namespace android {
namespace vts {

const static int kTcpPort = 5001;


// Starts to run a TCP server (foreground).
int StartTcpServer(const char* fuzzer_path32, const char* fuzzer_path64,
                   const char* spec_dir_path) {
  int sockfd, newsockfd;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    cerr << "Can't open the socket." << endl;
    return -1;
  }

  // Initialize socket structure
  bzero((char*) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(kTcpPort);

  // Now bind the host address using bind() call.
  if (::bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) {
    cerr << "Binding failed." << endl;
    return -1;
  }

  while (true) {
    // Now start listening for the clients, here process will go in sleep mode
    // and will wait for the incoming connection
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Accept actual connection from the client
    newsockfd = ::accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
    if (newsockfd < 0) {
      cerr << "Accept failed" << endl;
      return -1;
    }

    cout << "New session" << endl;
    pid_t pid = fork();
    if (pid == 0) {  // child
      AgentRequestHandler handler;
      exit(handler.StartSession(newsockfd, fuzzer_path32, fuzzer_path64,
                                spec_dir_path));
    } else if (pid < 0){
      cerr << "can't fork a child process to handle a session." << endl;
      return -1;
    }
  }
  return 0;
}

}  // namespace vts
}  // namespace android
