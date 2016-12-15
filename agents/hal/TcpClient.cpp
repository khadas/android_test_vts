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

#include "TcpClient.h"

#include <errno.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include <netdb.h>
#include <netinet/in.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <utils/RefBase.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "test/vts/proto/VtsDriverControlMessage.pb.h"

#include "BinderClient.h"
#include "RequestHandler.h"
#include "SocketUtil.h"

#define LOCALHOST_IP "127.0.0.1"

using namespace std;

namespace android {
namespace vts {

bool VtsDriverSocketClient::Connect() {
  struct sockaddr_in serv_addr;
  struct hostent* server;

  sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd_ < 0) {
    cerr << __func__ << " ERROR opening socket" << endl;
    sockfd_ = -1;
    return false;
  }
  server = gethostbyname(LOCALHOST_IP);
  if (server == NULL) {
    cerr << __func__ << " ERROR can't resolve the host name" << endl;
    sockfd_ = -1;
    return false;
  }
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char*) server->h_addr,
        (char*) &serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(server_port_);

  if (connect(sockfd_, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    cerr << __func__ << " ERROR connecting" << endl;
    sockfd_ = -1;
    return false;
  }
  return true;
}


bool VtsDriverSocketClient::Send(const string& message) {
  return VtsSocketSend(sockfd_, message);
}


void VtsDriverSocketClient::Close() {
  close(sockfd_);
}


int32_t VtsDriverSocketClient::LoadHal(
    const string& file_path, int target_class, int target_type,
    float target_version, const string& module_name) {
  VtsDriverControlCommandMessage* command_message =
      new VtsDriverControlCommandMessage();
  command_message->set_command_type(LOAD_HAL);
  command_message->set_file_path(file_path);
  command_message->set_target_class(target_class);
  command_message->set_target_type(target_type);
  command_message->set_target_version(target_version);
  command_message->set_module_name(module_name);

  string message;
  if (!command_message->SerializeToString(&message)) {
    cerr << "can't serialize the request message to a string." << endl;
    return -1;
  }

  if (!VtsSocketSend(sockfd_, message)) return -1;
  delete command_message;

  // receive response
  string response_string = VtsSocketRecv(sockfd_);
  if (response_string.length() == 0) return -1;

  VtsDriverControlResponseMessage response_message;
  if (!response_message.ParseFromString(message)) {
    cerr << __func__ << " can't parse the response message" << endl;
    return -1;
  }

  return response_message.response_code();
}


const char* VtsDriverSocketClient::GetFunctions() {
  VtsDriverControlCommandMessage* command_message =
      new VtsDriverControlCommandMessage();
  command_message->set_command_type(LIST_FUNCTIONS);

  string message;
  if (!command_message->SerializeToString(&message)) {
    cerr << "can't serialize the request message to a string." << endl;
    return NULL;
  }

  cout << "[agent->driver] LIST_FUNCTIONS len = " << message.length() << endl;
  if (!VtsSocketSend(sockfd_, message)) return NULL;
  delete command_message;

  // receive response
  string response_string = VtsSocketRecv(sockfd_);
  if (response_string.length() == 0) return NULL;

  cout << "[driver->agent] LIST_FUNCTIONS resp len = "
      << response_string.length() << endl;

  VtsDriverControlResponseMessage response_message;
  if (!response_message.ParseFromString(response_string)) {
    cerr << __func__ << " can't parse the message" << endl;
    return NULL;
  }

  char* result =
      (char*) malloc(strlen(response_message.return_message().c_str()) + 1);
  if (!result) {
    cerr << __func__ << " ERROR result is NULL" << endl;
    return NULL;
  }
  strcpy(result, response_message.return_message().c_str());
  return result;
}


const char* VtsDriverSocketClient::Call(const string& arg) {
  VtsDriverControlCommandMessage* command_message =
      new VtsDriverControlCommandMessage();
  command_message->set_command_type(CALL_FUNCTION);
  command_message->set_arg(arg);

  string message;
  if (!command_message->SerializeToString(&message)) {
    cerr << "can't serialize the request message to a string." << endl;
    return NULL;
  }

  if (!VtsSocketSend(sockfd_, message)) return NULL;
  delete command_message;

  // receive response
  string response_string = VtsSocketRecv(sockfd_);
  if (response_string.length() == 0) return NULL;

  VtsDriverControlResponseMessage response_message;
  if (!response_message.ParseFromString(response_string)) {
    cerr << __func__ << " can't parse the message" << endl;
    return NULL;
  }

  char* result =
      (char*) malloc(strlen(response_message.return_message().c_str()) + 1);
  if (!result) {
    cerr << __func__ << " ERROR result is NULL" << endl;
    return NULL;
  }
  strcpy(result, response_message.return_message().c_str());
  return result;
}


int32_t VtsDriverSocketClient::Status(int32_t type) {
  VtsDriverControlCommandMessage* command_message =
      new VtsDriverControlCommandMessage();
  command_message->set_command_type(CALL_FUNCTION);
  command_message->set_status_type(type);

  string message;
  if (!command_message->SerializeToString(&message)) {
    cerr << "can't serialize the request message to a string." << endl;
    return NULL;
  }

  if (!VtsSocketSend(sockfd_, message)) return NULL;
  delete command_message;

  // receive response
  string response_string = VtsSocketRecv(sockfd_);
  if (response_string.length() == 0) return NULL;

  VtsDriverControlResponseMessage response_message;
  if (!response_message.ParseFromString(message)) {
    cerr << __func__ << " can't parse the message" << endl;
    return NULL;
  }
  return response_message.return_value();
}


string GetSocketPortFilePath(const string& service_name) {
  string result("/data/local/tmp/");
  result += service_name;
  return result;
}


int GetSocketPortFromFile(const string& service_name, int retry_count) {
  string socket_port_flie_path = GetSocketPortFilePath(service_name);
  for (int retry = 0; retry < retry_count; retry++) {
    struct stat file_stat;
    if (stat(socket_port_flie_path.c_str(), &file_stat) == 0) {
      string line;
      ifstream socket_port_file(socket_port_flie_path.c_str());
      if (socket_port_file.is_open()) {
        getline(socket_port_file, line);
        socket_port_file.close();
        if (line.size() > 0) {
          return atoi(line.c_str());
        }
      }
    }
    if (retry_count != 0) sleep(1);
  }
  cout << __func__ << " " << "couldn't read the port number for "
      << service_name << endl;
  return -1;
}


VtsDriverSocketClient* GetDriverSocketClient(const string& service_name) {
  int port = GetSocketPortFromFile(service_name, 10);
  if (port == -1) return NULL;

  VtsDriverSocketClient* client = new VtsDriverSocketClient(port);
  if (!client->Connect()) {
    cerr << __func__ << " can't connect to " << port << endl;
    delete client;
    return NULL;
  }
  return client;
}

}  // namespace vts
}  // namespace android
