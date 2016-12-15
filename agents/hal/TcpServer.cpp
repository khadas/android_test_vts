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

#include <netdb.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>
#include <sstream>

#include "test/vts/agents/hal/proto/AndroidSystemControlMessage.pb.h"

using namespace std;

#define STATE_LENGTH 0
#define STATE_MESSAGE 1

namespace android {
namespace vts {

const static int kTcpPort = 5001;


int StartTcpServer() {
   int sockfd, newsockfd;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   int n;

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

   // If connection is established then start communicating
   char buffer[4096];
   bzero(buffer, 4096);
   char ch;
   int index = 0;
   int state = STATE_LENGTH;

   while (true) {
     if (read(newsockfd, &ch, 1) != 1) {
       cerr << "can't read any data" << endl;
       sleep(1);
       continue;
     }
     if (index == 4096) {
       cerr << "message too long (longer than " << index << " bytes)" << endl;
       return -1;
     }
     if (ch == '\n') {
       buffer[index] = '\0';
       int len = atoi(buffer);
       cout << "trying to read " << len << " bytes" << endl;
       if (read(newsockfd, buffer, len) != len) {
         cerr << "can't read all data" << endl;
         // TODO: handle by retrying
         return -1;
       }

       AndroidSystemControlCommandMessage command_msg;
       if (!command_msg.ParseFromString(string(buffer))) {
         cerr << "can't parse the cmd" << endl;
         return -1;
       }
       cout << "type " << command_msg.command_type() << endl;
       cout << "target_name " << command_msg.target_name() << endl;

       AndroidSystemControlResponseMessage response_msg;

       response_msg.set_response_code(SUCCESS);
       response_msg.set_reason("an example reason here");
       string response_msg_str;
       if (!response_msg.SerializeToString(&response_msg_str)) {
         cerr << "can't serialize the response message to a string." << endl;
         return -1;
       }

       // Write a response to the client
       std::stringstream response_header;
       response_header << response_msg_str.length() << "\n";
       cout << "sending '" << response_header.str() << "'" << endl;
       n = write(newsockfd, response_header.str().c_str(),
                 response_header.str().length());
       if (n < 0) {
         cerr << "ERROR writing to socket" << endl;
         return -1;
       }

       n = write(newsockfd, response_msg_str.c_str(), response_msg_str.length());
       cout << "sending '" << response_msg_str << "'" << endl;
       if (n < 0) {
         cerr << "ERROR writing to socket" << endl;
         return -1;
       }

       index = 0;
     } else {
       buffer[index++] = ch;
     }
   }

   return 0;
}

}  // namespace vts
}  // namespace android
