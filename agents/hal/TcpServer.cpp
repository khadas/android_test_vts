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

using namespace std;

#define MAX_FUZZER_ARGV_COUNT 30
#define TARGET_COMPONENT_DIR_PATH "/system/lib64/hw/"


namespace android {
namespace vts {

const static int kTcpPort = 5001;


AndroidSystemControlResponseMessage* RespondCheckFuzzerBinderService() {
  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();

  sp<IVtsFuzzer> binder = GetBinderClient();
  if (binder.get()) {
    response_msg->set_response_code(SUCCESS);
    response_msg->set_reason("an example success reason here");
  } else {
    response_msg->set_response_code(FAIL);
    response_msg->set_reason("an example failure reason here");
  }
  return response_msg;
}


AndroidSystemControlResponseMessage* RespondStartFuzzerBinderService(
    const string& args) {
  cout << "starting fuzzer" << endl;
  pid_t pid = fork();
  ResponseCode result = FAIL;
  if (pid == 0) {  // child
    cout << "Exec /system/bin/fuzzer " << args << endl;
    char* cmd;
    asprintf(&cmd, "/system/bin/fuzzer %s", args.c_str());
    system(cmd);
    cout << "fuzzer done" << endl;
    free(cmd);
    exit(0);
  } else if (pid > 0){
    for (int attempt = 0; attempt < 10; attempt++) {
      sleep(1);
      AndroidSystemControlResponseMessage* resp = RespondCheckFuzzerBinderService();
      if (resp->response_code() == SUCCESS) {
        result = SUCCESS;
        break;
      }
    }
  } else {
    cerr << "can't fork a child process to run the fuzzer." << endl;
    return NULL;
  }

  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();
  response_msg->set_response_code(result);
  response_msg->set_reason("an example success reason here");
  return response_msg;
}


AndroidSystemControlResponseMessage* RespondGetHals(const string& base_path) {
  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();

  ResponseCode result = FAIL;

  string files = "";

  DIR *dp;
  if (!(dp = opendir(base_path.c_str()))) {
    cerr << "Error(" << errno << ") opening " << TARGET_COMPONENT_DIR_PATH << endl;
    return NULL;
  }

  struct dirent* dirp;
  int len;
  while ((dirp = readdir(dp)) != NULL) {
    len = strlen(dirp->d_name);
    if (len > 3 && !strcmp(&dirp->d_name[len - 3], ".so")) {
      files += string(dirp->d_name) + " ";
      result = SUCCESS;
    }
  }
  closedir(dp);

  response_msg->set_reason(files);
  return response_msg;
}


AndroidSystemControlResponseMessage* RespondSelectHal(
    const string& file_name, int target_class, int target_type,
    float target_version) {
  // TODO: use an attribute (client) of a newly defined class.
  android::sp<android::vts::IVtsFuzzer> client = android::vts::GetBinderClient();
  if (!client.get()) return NULL;

  int32_t result = client->LoadHal(file_name, target_class, target_type,
                                   target_version);
  cout << "LoadHal: " << result << endl;

  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();
  if (result == 0) {
    response_msg->set_response_code(SUCCESS);
    response_msg->set_reason("Loaded the selected HAL.");
  } else {
    response_msg->set_response_code(FAIL);
    response_msg->set_reason("Failed to load the selected HAL.");
  }
  return response_msg;
}


AndroidSystemControlResponseMessage* RespondGetFunctions() {
  // TODO: use an attribute (client) of a newly defined class.
  android::sp<android::vts::IVtsFuzzer> client = android::vts::GetBinderClient();
  if (!client.get()) return NULL;

  const char* result = client->GetFunctions();
  cout << "GetFunctions: " << result << endl;

  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();
  if (result && strlen(result) > 0) {
    response_msg->set_response_code(SUCCESS);
    response_msg->set_reason(result);
  } else {
    response_msg->set_response_code(FAIL);
    response_msg->set_reason("Failed to load the selected HAL.");
  }
  return response_msg;
}


AndroidSystemControlResponseMessage* RespondCallFunction(const string& file_name) {
  // TODO: use an attribute (client) of a newly defined class.
  android::sp<android::vts::IVtsFuzzer> client = android::vts::GetBinderClient();
  if (!client.get()) return NULL;

  int v = 10;
  client->Status(v);
  const int32_t adder = 5;
  int32_t sum = client->Call(v, adder);
  cout << "Addition result: " << v << " + " << adder << " = " << sum << endl;
  client->Exit();
  return NULL;
}


AndroidSystemControlResponseMessage* RespondDefault() {
  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();

  response_msg->set_response_code(SUCCESS);
  response_msg->set_reason("an example reason here");
  return response_msg;
}


// handles a new session.
int HandleSession(int fd) {
  // If connection is established then start communicating
  char buffer[4096];
  char ch;
  int index = 0;
  int ret;
  int n;

  if (fd < 0) {
    cerr << "invalid fd " << fd << endl;
    return -1;
  }

  bzero(buffer, 4096);
  while (true) {
    if ((ret = read(fd, &ch, 1)) != 1) {
      cerr << "can't read any data. code = " << ret << endl;
      break;
    }
    if (index == 4096) {
      cerr << "message too long (longer than " << index << " bytes)" << endl;
      return -1;
    }
    if (ch == '\n') {
      buffer[index] = '\0';
      int len = atoi(buffer);
      cout << "trying to read " << len << " bytes" << endl;
      if (read(fd, buffer, len) != len) {
        cerr << "can't read all data" << endl;
        // TODO: handle by retrying
        return -1;
      }
      buffer[len] = '\0';

      AndroidSystemControlCommandMessage command_msg;
      for (int i = 0; i < len; i++) {
        cout << int(buffer[i]) << " ";
      }
      cout << endl;
      if (!command_msg.ParseFromString(string(buffer))) {
        cerr << "can't parse the cmd" << endl;
        return -1;
      }
      cout << "type " << command_msg.command_type() << endl;
      cout << "target_name " << command_msg.target_name() << endl;

      AndroidSystemControlResponseMessage* response_msg = NULL;
      switch (command_msg.command_type()) {
        case CHECK_FUZZER_BINDER_SERVICE:
          response_msg = RespondCheckFuzzerBinderService();
          break;
        case START_FUZZER_BINDER_SERVICE:
          response_msg = RespondStartFuzzerBinderService(
              command_msg.target_name());
          break;
        case GET_HALS:
          if (command_msg.target_name().length() > 0) {
            response_msg = RespondGetHals(command_msg.target_name());
          } else {
            response_msg = RespondGetHals(TARGET_COMPONENT_DIR_PATH);
          }
          break;
        case SELECT_HAL:
          response_msg = RespondSelectHal(command_msg.target_name(),
                                          command_msg.target_class(),
                                          command_msg.target_type(),
                                          command_msg.target_version() / 100.0);
          break;
        case GET_FUNCTIONS:
          response_msg = RespondGetFunctions();
          break;
        case CALL_FUNCTION:
          response_msg = RespondCallFunction(command_msg.target_name());
          break;
        default:
          response_msg = RespondDefault();
          break;
      }

      if (!response_msg) {
        cerr << "response_msg == NULL" << endl;
        return -1;
      }

      string response_msg_str;
      if (!response_msg->SerializeToString(&response_msg_str)) {
        cerr << "can't serialize the response message to a string." << endl;
        return -1;
      }

      // Write a response to the client
      std::stringstream response_header;
      response_header << response_msg_str.length() << "\n";
      cout << "sending '" << response_header.str() << "'" << endl;
      n = write(fd, response_header.str().c_str(),
                response_header.str().length());
      if (n < 0) {
        cerr << "ERROR writing to socket" << endl;
        return -1;
      }

      n = write(fd, response_msg_str.c_str(), response_msg_str.length());
      cout << "sending '" << response_msg_str << "'" << endl;
      if (n < 0) {
        cerr << "ERROR writing to socket" << endl;
        return -1;
      }

      delete response_msg;
      index = 0;
    } else {
      buffer[index++] = ch;
    }
  }
  return 0;
}


// Starts to run a TCP server (foreground).
int StartTcpServer() {
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
      exit(HandleSession(newsockfd));
    } else if (pid < 0){
      cerr << "can't fork a child process to handle a session." << endl;
      return -1;
    }
  }

  return 0;
}

}  // namespace vts
}  // namespace android
