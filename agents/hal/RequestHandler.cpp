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

#include "RequestHandler.h"

#include <errno.h>

#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utils/RefBase.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "test/vts/runners/host/proto/AndroidSystemControlMessage.pb.h"
#include "test/vts/runners/host/proto/InterfaceSpecificationMessage.pb.h"
#include "BinderClient.h"

using namespace std;
using namespace google::protobuf;

namespace android {
namespace vts {

AndroidSystemControlResponseMessage* AgentRequestHandler::ListHals(
    const RepeatedPtrField<string>& base_paths) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();
  ResponseCode result = FAIL;

  for (const string& path : base_paths) {
    cout << __FUNCTION__ << ": open a dir " << path << endl;
    DIR* dp;
    if (!(dp = opendir(path.c_str()))) {
      cerr << "Error(" << errno << ") opening " << path << endl;
      continue;
    }
    struct dirent* dirp;
    int len;
    while ((dirp = readdir(dp)) != NULL) {
      len = strlen(dirp->d_name);
      if (len > 3 && !strcmp(&dirp->d_name[len - 3], ".so")) {
        string found_path = path + "/" + string(dirp->d_name);
        cout << __FUNCTION__ << ": found " << found_path << endl;
        response_msg->add_file_names(found_path);
        result = SUCCESS;
      }
    }
    closedir(dp);
  }
  response_msg->set_response_code(result);
  return response_msg;
}


AndroidSystemControlResponseMessage* AgentRequestHandler::CheckStubService(
    const string& service_name) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();

  sp<IVtsFuzzer> binder = GetBinderClient(service_name);
  if (binder.get()) {
    response_msg->set_response_code(SUCCESS);
    response_msg->set_reason("found the service");
    cout << "set service_name " << service_name << endl;
    service_name_ = service_name;
  } else {
    response_msg->set_response_code(FAIL);
    response_msg->set_reason("service not found");
  }
  return response_msg;
}


AndroidSystemControlResponseMessage* AgentRequestHandler::LaunchStubService(
    const char* spec_dir_path, const string& service_name,
    const string& file_path, const char* fuzzer_path,
    int target_class, int target_type,
    float target_version, const string& module_name) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
  pid_t pid = fork();
  ResponseCode result = FAIL;

  // TODO: shall check whether there's a service with the same name and return
  // success immediately if exists.
  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();

  if (pid == 0) {  // child
    char* cmd;
    string fuzzer_path_string(fuzzer_path);
    size_t offset = fuzzer_path_string.find_last_of("/");
    string ld_dir_path = fuzzer_path_string.substr(0, offset);
    if (!spec_dir_path) {
      asprintf(
          &cmd,
          "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --server --service_name=%s",
          ld_dir_path.c_str(), fuzzer_path, service_name.c_str());
    } else {
      asprintf(
          &cmd,
          "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --server --service_name=%s --spec_dir=%s",
          ld_dir_path.c_str(), fuzzer_path, service_name.c_str(), spec_dir_path);
    }
    cout << "Exec " << cmd << endl;
    system(cmd);
    cout << "fuzzer exits" << endl;
    free(cmd);
    exit(0);
  } else if (pid > 0){
    for (int attempt = 0; attempt < 10; attempt++) {
      sleep(1);
      AndroidSystemControlResponseMessage* resp =
          CheckStubService(service_name);
      if (resp->response_code() == SUCCESS) {
        result = SUCCESS;
        break;
      }
    }
    if (result) {
      // TODO: use an attribute (client) of a newly defined class.
      android::sp<android::vts::IVtsFuzzer> client =
          android::vts::GetBinderClient(service_name);
      if (!client.get()) {
        response_msg->set_response_code(FAIL);
        response_msg->set_reason("Failed to start a stub.");
        return response_msg;  // TODO: kill the stub?
      }
      cout << "[agent->stub]: LoadHal " << module_name << endl;
      int32_t result = client->LoadHal(
          file_path, target_class, target_type, target_version, module_name);
      cout << "[stub->agent]: LoadHal returns " << result << endl;
      if (result == 0) {
        response_msg->set_response_code(SUCCESS);
        response_msg->set_reason("Loaded the selected HAL.");
        cout << "set service_name " << service_name << endl;
        service_name_ = service_name;
      } else {
        response_msg->set_response_code(FAIL);
        response_msg->set_reason("Failed to load the selected HAL.");
      }
      return response_msg;
    }
  }
  response_msg->set_response_code(FAIL);
  response_msg->set_reason("Failed to fork a child process to start a stub.");
  cerr << "can't fork a child process to run the fuzzer." << endl;
  return response_msg;
}


AndroidSystemControlResponseMessage* AgentRequestHandler::ListApis() {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
  // TODO: use an attribute (client) of a newly defined class.
  android::sp<android::vts::IVtsFuzzer> client = android::vts::GetBinderClient(
      service_name_);
  if (!client.get()) return NULL;

  const char* result = client->GetFunctions();
  cout << "GetFunctions: len " << strlen(result) << endl;
  // if (result) cout << "GetFunctions: " << result << endl;

  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();
  if (result != NULL && strlen(result) > 0) {
    response_msg->set_response_code(SUCCESS);
    response_msg->set_spec(result);
  } else {
    response_msg->set_response_code(FAIL);
    response_msg->set_reason("Failed to get the functions.");
  }
  return response_msg;
}


AndroidSystemControlResponseMessage* AgentRequestHandler::CallApi(
    const string& call_payload) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
  // TODO: use an attribute (client) of a newly defined class.
  android::sp<android::vts::IVtsFuzzer> client = android::vts::GetBinderClient(
      service_name_);
  if (!client.get()) return NULL;

  const char* result = client->Call(call_payload);
  //if (result) cout << "Call: " << result << endl;

  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();
  if (result != NULL && strlen(result) > 0) {
    cout << "Call: success" << endl;
    response_msg->set_response_code(SUCCESS);
    response_msg->set_result(result);
  } else {
    cout << "Call: fail" << endl;
    response_msg->set_response_code(FAIL);
    response_msg->set_reason("Failed to call the api.");
  }
  return response_msg;
}


AndroidSystemControlResponseMessage* AgentRequestHandler::DefaultResponse() {
  cout << "[agent] " << __FUNCTION__ << endl;
  AndroidSystemControlResponseMessage* response_msg =
      new AndroidSystemControlResponseMessage();

  response_msg->set_response_code(SUCCESS);
  response_msg->set_reason("an example reason here");
  return response_msg;
}

#define RX_BUF_SIZE (128 * 1024)

// handles a new session.
int AgentRequestHandler::StartSession(
    int fd, const char* fuzzer_path32, const char* fuzzer_path64,
    const char* spec_dir_path) {
  // If connection is established then start communicating
  char buffer[RX_BUF_SIZE];
  char ch;
  int index = 0;
  int ret;
  int n;

  if (fd < 0) {
    cerr << "invalid fd " << fd << endl;
    return -1;
  }

  bzero(buffer, RX_BUF_SIZE);
  while (true) {
    if ((ret = read(fd, &ch, 1)) != 1) {
      cerr << "ERROR:pid(" << getpid() << "): can't read any data. code = "
          << ret << endl;
      break;
    }
    if (index == RX_BUF_SIZE) {
      cerr << "message too long (longer than " << index << " bytes)" << endl;
      return -1;
    }
    if (ch == '\n') {
      buffer[index] = '\0';
      int len = atoi(buffer);
      // cout << "trying to read " << len << " bytes" << endl;
      if (read(fd, buffer, len) != len) {
        cerr << "can't read all data" << endl;
        // TODO: handle by retrying
        return -1;
      }
      buffer[len] = '\0';

      AndroidSystemControlCommandMessage command_msg;
      //for (int i = 0; i < len; i++) {
      //  cout << int(buffer[i]) << " ";
      //}
      //cout << endl;
      if (!command_msg.ParseFromString(string(buffer))) {
        cerr << "can't parse the cmd" << endl;
        return -1;
      }
      //cout << "[runner->agent] command type " << command_msg.command_type()
      //    << endl;

      AndroidSystemControlResponseMessage* response_msg = NULL;
      switch (command_msg.command_type()) {
        case LIST_HALS:
          response_msg = ListHals(command_msg.paths());
          break;
        case CHECK_STUB_SERVICE:
          response_msg = CheckStubService(command_msg.service_name());
          break;
        case LAUNCH_STUB_SERVICE:
          const char* fuzzer_path;
          if (command_msg.bits() == 32) {
            fuzzer_path = fuzzer_path32;
          } else {
            fuzzer_path = fuzzer_path64;
          }
          response_msg = LaunchStubService(
              spec_dir_path,
              command_msg.service_name(),
              command_msg.file_path(),
              fuzzer_path,
              command_msg.target_class(),
              command_msg.target_type(),
              command_msg.target_version() / 100.0,
              command_msg.module_name());
          break;
        case LIST_APIS:
          response_msg = ListApis();
          break;
        case CALL_API:
          response_msg = CallApi(command_msg.arg());
          break;
        default:
          response_msg = DefaultResponse();
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
      // cout << "sending '" << response_header.str() << "'" << endl;
      n = write(fd, response_header.str().c_str(),
                response_header.str().length());
      if (n < 0) {
        cerr << "ERROR writing to socket" << endl;
        return -1;
      }

      n = write(fd, response_msg_str.c_str(), response_msg_str.length());
      cout << "[agent->runner] reply " << response_msg_str.length() << " bytes" << endl;
      // cout << "sending '" << response_msg_str << "'" << endl;
      if (n < 0) {
        cerr << "[agent->runner] ERROR writing to socket" << endl;
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


}  // namespace vts
}  // namespace android
