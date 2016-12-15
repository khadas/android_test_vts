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

#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket

#include "SocketServer.h"

#define LOG_TAG "VtsDriverHalSocketServer"
#include <utils/Log.h>
#include <utils/String8.h>

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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <google/protobuf/text_format.h>
#include "test/vts/proto/VtsDriverControlMessage.pb.h"

#include "binder/VtsFuzzerBinderService.h"
#include "specification_parser/SpecificationBuilder.h"

using namespace std;

#define MAX_HEADER_BUFFER_SIZE 128


namespace android {
namespace vts {


void VtsDriverHalSocketServer::Exit() {
  printf("VtsFuzzerServer::Exit\n");
}


int32_t VtsDriverHalSocketServer::LoadHal(
    const string& path, int target_class, int target_type,
    float target_version, const string& module_name) {
  printf("VtsFuzzerServer::LoadHal(%s)\n", path.c_str());
  bool success = spec_builder_.LoadTargetComponent(
      path.c_str(), lib_path_, target_class, target_type, target_version,
      module_name.c_str());
  cout << "Result: " << success << std::endl;
  if (success) {
    return 0;
  } else {
    return -1;
  }
}


int32_t VtsDriverHalSocketServer::Status(int32_t type) {
  printf("VtsFuzzerServer::Status(%i)\n", type);
  return 0;
}


const char* VtsDriverHalSocketServer::Call(const string& arg) {
  printf("VtsFuzzerServer::Call(%s)\n", arg.c_str());
  FunctionSpecificationMessage* func_msg = new FunctionSpecificationMessage();
  google::protobuf::TextFormat::MergeFromString(arg, func_msg);
  printf("%s: call!!!\n", __func__);
  const string& result = spec_builder_.CallFunction(func_msg);
  printf("call done!!!\n");
  return result.c_str();
}


const char* VtsDriverHalSocketServer::GetFunctions() {
  printf("Get functions*");
  vts::InterfaceSpecificationMessage* spec =
      spec_builder_.GetInterfaceSpecification();
  if (!spec) {
    return NULL;
  }
  string* output = new string();
  printf("getfunctions serial1\n");
  if (google::protobuf::TextFormat::PrintToString(*spec, output)) {
    printf("getfunctions length %d\n", output->length());
    return output->c_str();
  } else {
    printf("can't serialize the interface spec message to a string.\n");
    return NULL;
  }
}


// Starts to run a TCP server (foreground).
int VtsDriverHalSocketServer::Start(const string& socket_port_file) {
  int sockfd;
  int newsockfd;
  socklen_t clilen;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    cerr << "Can't open the socket." << endl;
    return -1;
  }

  // Initialize socket structure
  bzero((char*) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = 0;

  // Now bind the host address using bind() call.
  if (::bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) {
    cerr << "Binding failed." << endl;
    return -1;
  }

  socklen_t len = sizeof(serv_addr);
  if (getsockname(sockfd, (struct sockaddr*) &serv_addr, &len) == -1) {
    cerr << __func__ << " Couldn't get socket name." << endl;
    return -1;
  }

  ofstream port_file;
  port_file.open(socket_port_file);
  port_file << ntohs(serv_addr.sin_port);
  port_file.close();
  cout << __func__ << " wrote the port to a file." << endl;

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
      while(StartSession(newsockfd));
      exit(0);
    } else if (pid < 0) {
      cerr << "can't fork a child process to handle a session." << endl;
      return -1;
    }
  }
  cerr << "[driver] exiting" << endl;
  return 0;
}


bool VtsDriverHalSocketServer::Send(int sockfd, const string& message) {
  std::stringstream header;
  header << message.length() << "\n";
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
  return true;
}


bool VtsDriverHalSocketServer::StartSession(int sockfd) {
  int header_index = 0;
  char header_buffer[MAX_HEADER_BUFFER_SIZE];

  for (header_index = 0; header_index < MAX_HEADER_BUFFER_SIZE;
       header_index++) {
    if (read(sockfd, &header_buffer[header_index], 1) != 1) {
      cerr << __func__ << " ERROR reading the length" << endl;
      return false;
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
    return false;
  }

  if (read(sockfd, msg, msg_len) !=  msg_len) {
    cerr << __func__ << " ERROR read failed" << endl;
    return false;
  }
  msg[msg_len] = '\0';

  VtsDriverControlCommandMessage command_message;
  if (!command_message.ParseFromString(string(msg, msg_len))) {
    cerr << __func__ << " can't parse the message, length = "
        << msg_len << endl;
    return false;
  }

  switch(command_message.command_type()) {
    case EXIT: {
      Exit();
      VtsDriverControlResponseMessage* response_message =
          new VtsDriverControlResponseMessage();
      response_message->set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);

      string message;
      if (!response_message->SerializeToString(&message)) {
        cerr << "can't serialize the request message to a string." << endl;
        break;
      }
      delete response_message;

      if (Send(sockfd, message)) return true;
      break;
    }
    case LOAD_HAL: {
      int32_t result = LoadHal(command_message.file_path(),
                               command_message.target_class(),
                               command_message.target_type(),
                               command_message.target_version(),
                               command_message.module_name());

      VtsDriverControlResponseMessage* response_message =
          new VtsDriverControlResponseMessage();
      response_message->set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      response_message->set_return_value(result);

      string message;
      if (!response_message->SerializeToString(&message)) {
        cerr << "can't serialize the request message to a string." << endl;
        break;
      }
      delete response_message;

      if (Send(sockfd, message)) return true;
      break;
    }
    case GET_STATUS: {
      int32_t result = Status(command_message.status_type());

      VtsDriverControlResponseMessage* response_message =
          new VtsDriverControlResponseMessage();
      response_message->set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      response_message->set_return_value(result);

      string message;
      if (!response_message->SerializeToString(&message)) {
        cerr << "can't serialize the request message to a string." << endl;
        break;
      }
      delete response_message;

      if (Send(sockfd, message)) return true;
      break;
    }
    case CALL_FUNCTION: {
      const char* result = Call(command_message.arg());

      VtsDriverControlResponseMessage* response_message =
          new VtsDriverControlResponseMessage();
      response_message->set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      response_message->set_return_message(result);

      string message;
      if (!response_message->SerializeToString(&message)) {
        cerr << "can't serialize the request message to a string." << endl;
        break;
      }
      delete response_message;

      if (Send(sockfd, message)) return true;
      break;
    }
    case LIST_FUNCTIONS: {
      const char* result = GetFunctions();

      VtsDriverControlResponseMessage* response_message =
                new VtsDriverControlResponseMessage();
      response_message->set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      response_message->set_return_message(result);

      string message;
      if (!response_message->SerializeToString(&message)) {
        cerr << "can't serialize the request message to a string." << endl;
        break;
      }
      delete response_message;

      if (Send(sockfd, message)) return true;
      break;
    }
  }
  return false;
}


void StartSocketServer(const string& socket_port_file,
                       android::vts::SpecificationBuilder& spec_builder,
                       const char* lib_path) {
  VtsDriverHalSocketServer* server =
      new VtsDriverHalSocketServer(spec_builder, lib_path);
  server->Start(socket_port_file);
}

}  // namespace vts
}  // namespace android

#endif
