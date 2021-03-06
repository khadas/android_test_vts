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

#include "AgentRequestHandler.h"

#include <errno.h>

#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/RefBase.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "BinderClientToDriver.h"
#include "SocketClientToDriver.h"
#include "SocketServerForDriver.h"
#include "test/vts/proto/AndroidSystemControlMessage.pb.h"
#include "test/vts/proto/VtsDriverControlMessage.pb.h"

using namespace std;
using namespace google::protobuf;

namespace android {
namespace vts {

bool AgentRequestHandler::ListHals(const RepeatedPtrField<string>& base_paths) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
  AndroidSystemControlResponseMessage response_msg;
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
        response_msg.add_file_names(found_path);
        result = SUCCESS;
      }
    }
    closedir(dp);
  }
  response_msg.set_response_code(result);
  return VtsSocketSendMessage(response_msg);
}

bool AgentRequestHandler::SetHostInfo(const int callback_port) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
  callback_port_ = callback_port;
  AndroidSystemControlResponseMessage response_msg;
  response_msg.set_response_code(SUCCESS);
  return VtsSocketSendMessage(response_msg);
}

bool AgentRequestHandler::CheckDriverService(const string& service_name,
                                             bool* live) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
  AndroidSystemControlResponseMessage response_msg;

#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  if (IsDriverRunning(service_name, 10)) {
#else  // binder
  sp<IVtsFuzzer> binder = GetBinderClient(service_name);
  if (binder.get()) {
#endif
    if (live) *live = true;
    response_msg.set_response_code(SUCCESS);
    response_msg.set_reason("found the service");
    cout << "set service_name " << service_name << endl;
    service_name_ = service_name;
  } else {
    if (live) *live = false;
    response_msg.set_response_code(FAIL);
    response_msg.set_reason("service not found");
  }
  return VtsSocketSendMessage(response_msg);
}

static const char kUnixSocketNamePrefixForCallbackServer[] =
    "/data/local/tmp/vts_agent_callback";

bool AgentRequestHandler::LaunchDriverService(
    const AndroidSystemControlCommandMessage& command_msg) {
  int driver_type = command_msg.driver_type();
  const string& service_name = command_msg.service_name();
  const string& file_path = command_msg.file_path();
  int target_class = command_msg.target_class();
  int target_type = command_msg.target_type();
  float target_version = command_msg.target_version() / 100.0;
  const string& target_package = command_msg.target_package();
  const string& target_component_name = command_msg.target_component_name();
  const string& module_name = command_msg.module_name();
  const string& hw_binder_service_name = command_msg.hw_binder_service_name();
  int bits = command_msg.bits();

  cout << "[runner->agent] command " << __FUNCTION__ << " (file_path="
       << file_path << ")" << endl;
  ResponseCode result = FAIL;

  // TODO: shall check whether there's a service with the same name and return
  // success immediately if exists.
  AndroidSystemControlResponseMessage response_msg;

  // deletes the service file if exists before starting to launch a driver.
  string socket_port_flie_path = GetSocketPortFilePath(service_name);
  struct stat file_stat;
  if (stat(socket_port_flie_path.c_str(), &file_stat) == 0  // file exists
      && remove(socket_port_flie_path.c_str()) == -1) {
    cerr << __func__ << " " << socket_port_flie_path << " delete error" << endl;
    response_msg.set_reason("service file already exists.");
  } else {
    pid_t pid = fork();
    if (pid == 0) {  // child
      Close();

      string driver_binary_path;
      char* cmd = NULL;
      if (driver_type == VTS_DRIVER_TYPE_HAL_CONVENTIONAL ||
          driver_type == VTS_DRIVER_TYPE_HAL_LEGACY ||
          driver_type == VTS_DRIVER_TYPE_HAL_HIDL) {
        // TODO: check whether the port is available and handle if fails.
        static int port = 0;
        string callback_socket_name(kUnixSocketNamePrefixForCallbackServer);
        callback_socket_name += std::to_string(port++);
        cout << "callback_socket_name: " << callback_socket_name << endl;
        StartSocketServerForDriver(callback_socket_name, -1);

        if (bits == 32) {
          driver_binary_path = driver_hal_binary32_;
        } else {
          driver_binary_path = driver_hal_binary64_;
        }
        size_t offset = driver_binary_path.find_last_of("/");
        string ld_dir_path = driver_binary_path.substr(0, offset);

        if (driver_hal_spec_dir_path_.length() < 1) {
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
          asprintf(&cmd,
                   "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --server "
                   "--server_socket_path=%s "
                   "--callback_socket_name=%s",
                   ld_dir_path.c_str(), driver_binary_path.c_str(),
                   socket_port_flie_path.c_str(), callback_socket_name.c_str());
#else  // binder
          asprintf(&cmd,
                   "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --server "
                   "--service_name=%s "
                   "--callback_socket_name=%s",
                   ld_dir_path.c_str(), driver_binary_path.c_str(),
                   service_name.c_str(), callback_socket_name.c_str());
#endif
        } else {
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
          asprintf(&cmd,
                   "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --server "
                   "--server_socket_path=%s "
                   "--spec_dir=%s --callback_socket_name=%s",
                   ld_dir_path.c_str(), driver_binary_path.c_str(),
                   socket_port_flie_path.c_str(),
                   driver_hal_spec_dir_path_.c_str(),
                   callback_socket_name.c_str());
#else  // binder
          asprintf(&cmd,
                   "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --server "
                   "--service_name=%s "
                   "--spec_dir=%s --callback_socket_name=%s",
                   ld_dir_path.c_str(), driver_binary_path.c_str(),
                   service_name.c_str(), driver_hal_spec_dir_path_.c_str(),
                   callback_socket_name.c_str());
#endif
        }
      } else if (driver_type == VTS_DRIVER_TYPE_SHELL) {
        if (bits == 32) {
          driver_binary_path = driver_shell_binary32_;
        } else {
          driver_binary_path = driver_shell_binary64_;
        }
        size_t offset = driver_binary_path.find_last_of("/");
        string ld_dir_path = driver_binary_path.substr(0, offset);

#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
        asprintf(
            &cmd,
            "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --server_socket_path=%s",
            ld_dir_path.c_str(), driver_binary_path.c_str(),
            socket_port_flie_path.c_str());
#else  // binder
        cerr << __func__ << " no binder implementation available." << endl;
        exit(-1);
#endif
      } else {
        cerr << __func__ << " unsupported driver type." << endl;
      }

      if (cmd) {
        cout << __func__ << "launch a driver - " << cmd << endl;
        system(cmd);
        cout << __func__ << "driver exits" << endl;
        free(cmd);
      }
      exit(0);
    } else if (pid > 0) {
      for (int attempt = 0; attempt < 10; attempt++) {
        sleep(1);
        if (IsDriverRunning(service_name, 10)) {
          result = SUCCESS;
          break;
        }
      }
      if (result) {
// TODO: use an attribute (client) of a newly defined class.
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
        VtsDriverSocketClient* client =
            android::vts::GetDriverSocketClient(service_name);
        if (!client) {
#else  // binder
        android::sp<android::vts::IVtsFuzzer> client =
            android::vts::GetBinderClient(service_name);
        if (!client.get()) {
#endif
          response_msg.set_response_code(FAIL);
          response_msg.set_reason("Failed to start a driver.");
          // TODO: kill the driver?
          return VtsSocketSendMessage(response_msg);
        }
        int32_t result;
        if (driver_type == VTS_DRIVER_TYPE_HAL_CONVENTIONAL ||
            driver_type == VTS_DRIVER_TYPE_HAL_LEGACY ||
            driver_type == VTS_DRIVER_TYPE_HAL_HIDL) {
          cout << "[agent->driver]: LoadHal " << module_name << endl;
          result = client->LoadHal(file_path, target_class, target_type,
                                   target_version, target_package,
                                   target_component_name,
                                   hw_binder_service_name, module_name);
          cout << "[driver->agent]: LoadHal returns " << result << endl;
          if (result == VTS_DRIVER_RESPONSE_SUCCESS) {
            response_msg.set_response_code(SUCCESS);
            response_msg.set_reason("Loaded the selected HAL.");
            cout << "set service_name " << service_name << endl;
            service_name_ = service_name;
          } else {
            response_msg.set_response_code(FAIL);
            response_msg.set_reason("Failed to load the selected HAL.");
          }
        } else if (driver_type == VTS_DRIVER_TYPE_SHELL) {
          response_msg.set_response_code(SUCCESS);
          response_msg.set_reason("Loaded the shell driver.");
          cout << "set service_name " << service_name << endl;
          service_name_ = service_name;
        }

#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
        driver_client_ = client;
#endif
        return VtsSocketSendMessage(response_msg);
      }
    }
    response_msg.set_reason(
        "Failed to fork a child process to start a driver.");
  }
  response_msg.set_response_code(FAIL);
  cerr << "can't fork a child process to run the vts_hal_driver." << endl;
  return VtsSocketSendMessage(response_msg);
}

bool AgentRequestHandler::ReadSpecification(
    const AndroidSystemControlCommandMessage& command_message) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  VtsDriverSocketClient* client = driver_client_;
  if (!client) {
#else  // binder
  android::sp<android::vts::IVtsFuzzer> client =
      android::vts::GetBinderClient(service_name_);
  if (!client.get()) {
#endif
    return false;
  }

  const char* result = client->ReadSpecification(
      command_message.service_name(),
      command_message.target_class(),
      command_message.target_type(),
      command_message.target_version() / 100.0,
      command_message.target_package());

  AndroidSystemControlResponseMessage response_msg;
  if (result != NULL && strlen(result) > 0) {
    cout << "[agent] Call: success" << endl;
    response_msg.set_response_code(SUCCESS);
    response_msg.set_result(result);
  } else {
    cout << "[agent] Call: fail" << endl;
    response_msg.set_response_code(FAIL);
    response_msg.set_reason("Failed to call the api.");
  }
  bool succ = VtsSocketSendMessage(response_msg);
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  free((void*)result);
#endif
  return succ;
}

bool AgentRequestHandler::ListApis() {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
// TODO: use an attribute (client) of a newly defined class.
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  VtsDriverSocketClient* client = driver_client_;
  if (!client) {
#else  // binder
  android::sp<android::vts::IVtsFuzzer> client =
      android::vts::GetBinderClient(service_name_);
  if (!client.get()) {
#endif
    return false;
  }
  const char* result = client->GetFunctions();
  if (result != NULL) {
    cout << "GetFunctions: len " << strlen(result) << endl;
  }

  AndroidSystemControlResponseMessage response_msg;
  if (result != NULL && strlen(result) > 0) {
    response_msg.set_response_code(SUCCESS);
    response_msg.set_spec(string(result));
  } else {
    response_msg.set_response_code(FAIL);
    response_msg.set_reason("Failed to get the functions.");
  }

  bool succ = VtsSocketSendMessage(response_msg);
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  free((void*)result);
#endif
  return succ;
}

bool AgentRequestHandler::CallApi(const string& call_payload,
                                  const string& uid) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  VtsDriverSocketClient* client = driver_client_;
  if (!client) {
#else  // binder
  // TODO: use an attribute (client) of a newly defined class.
  android::sp<android::vts::IVtsFuzzer> client =
      android::vts::GetBinderClient(service_name_);
  if (!client.get()) {
#endif
    return false;
  }

  const char* result = client->Call(call_payload, uid);

  AndroidSystemControlResponseMessage response_msg;
  if (result != NULL && strlen(result) > 0) {
    cout << "[agent] Call: success" << endl;
    response_msg.set_response_code(SUCCESS);
    response_msg.set_result(result);
  } else {
    cout << "[agent] Call: fail" << endl;
    response_msg.set_response_code(FAIL);
    response_msg.set_reason("Failed to call the api.");
  }
  bool succ = VtsSocketSendMessage(response_msg);
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  free((void*)result);
#endif
  return succ;
}

bool AgentRequestHandler::GetAttribute(const string& payload) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  VtsDriverSocketClient* client = driver_client_;
  if (!client) {
#else  // binder
  // TODO: use an attribute (client) of a newly defined class.
  android::sp<android::vts::IVtsFuzzer> client =
      android::vts::GetBinderClient(service_name_);
  if (!client.get()) {
#endif
    return false;
  }

  const char* result = client->GetAttribute(payload);

  AndroidSystemControlResponseMessage response_msg;
  if (result != NULL && strlen(result) > 0) {
    cout << "[agent] Call: success" << endl;
    response_msg.set_response_code(SUCCESS);
    response_msg.set_result(result);
  } else {
    cout << "[agent] Call: fail" << endl;
    response_msg.set_response_code(FAIL);
    response_msg.set_reason("Failed to call the api.");
  }
  bool succ = VtsSocketSendMessage(response_msg);
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  free((void*)result);
#endif
  return succ;
}

bool AgentRequestHandler::DefaultResponse() {
  cout << "[agent] " << __FUNCTION__ << endl;
  AndroidSystemControlResponseMessage response_msg;
  response_msg.set_response_code(SUCCESS);
  response_msg.set_reason("an example reason here");
  return VtsSocketSendMessage(response_msg);
}

bool AgentRequestHandler::ExecuteShellCommand(
    const AndroidSystemControlCommandMessage& command_message) {
  cout << "[runner->agent] command " << __FUNCTION__ << endl;
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  VtsDriverSocketClient* client = driver_client_;
  if (!client) {
#else  // binder
  cerr << __func__ << " binder not supported." << endl;
  {
#endif
    return false;
  }

  VtsDriverControlResponseMessage* result_message = client->ExecuteShellCommand(
      command_message.shell_command());

  AndroidSystemControlResponseMessage response_msg;

  if (result_message == NULL) {
    cout << "ExecuteShellCommand: failed to call the api" << endl;
    response_msg.set_response_code(FAIL);
    response_msg.set_reason("Failed to call the api.");
  } else {
    CreateSystemControlResponseFromDriverControlResponse(
        *result_message, &response_msg);
    delete(result_message);
  }

  bool succ = VtsSocketSendMessage(response_msg);
  return succ;
}

void AgentRequestHandler::CreateSystemControlResponseFromDriverControlResponse(
    const VtsDriverControlResponseMessage& driver_control_response_message,
    AndroidSystemControlResponseMessage* system_control_response_message) {

  if (driver_control_response_message.response_code() ==
      VTS_DRIVER_RESPONSE_SUCCESS) {
    cout << "ExecuteShellCommand: shell driver reported success" << endl;
    system_control_response_message->set_response_code(SUCCESS);
  } else if (driver_control_response_message.response_code() ==
      VTS_DRIVER_RESPONSE_FAIL) {
    cout << "ExecuteShellCommand: shell driver reported fail" << endl;
    system_control_response_message->set_response_code(FAIL);
  } else if (driver_control_response_message.response_code() ==
      UNKNOWN_VTS_DRIVER_RESPONSE_CODE) {
    cout << "ExecuteShellCommand: shell driver reported unknown" << endl;
    system_control_response_message->set_response_code(UNKNOWN_RESPONSE_CODE);
  }

  for (const auto& log_stdout : driver_control_response_message.stdout()) {
    system_control_response_message->add_stdout(log_stdout);
  }

  for (const auto& log_stderr : driver_control_response_message.stderr()) {
    system_control_response_message->add_stderr(log_stderr);
  }

  for (const auto& exit_code : driver_control_response_message.exit_code()) {
    system_control_response_message->add_exit_code(exit_code);
  }
}

bool AgentRequestHandler::ProcessOneCommand() {
  AndroidSystemControlCommandMessage command_msg;
  if (!VtsSocketRecvMessage(&command_msg)) return false;

  cout << getpid() << " " << __func__
       << " command_type = " << command_msg.command_type() << endl;
  switch (command_msg.command_type()) {
    case LIST_HALS:
      return ListHals(command_msg.paths());
    case SET_HOST_INFO:
      return SetHostInfo(command_msg.callback_port());
    case CHECK_DRIVER_SERVICE:
      return CheckDriverService(command_msg.service_name(), NULL);
    case LAUNCH_DRIVER_SERVICE:
      return LaunchDriverService(command_msg);
    case VTS_AGENT_COMMAND_READ_SPECIFICATION:
      return ReadSpecification(command_msg);
    case LIST_APIS:
      return ListApis();
    case CALL_API:
      return CallApi(command_msg.arg(), command_msg.driver_caller_uid());
    case VTS_AGENT_COMMAND_GET_ATTRIBUTE:
      return GetAttribute(command_msg.arg());
    // for shell driver
    case VTS_AGENT_COMMAND_EXECUTE_SHELL_COMMAND:
      ExecuteShellCommand(command_msg);
      return true;
    default:
      cerr << __func__ << " ERROR unknown command "
           << command_msg.command_type() << endl;
      return DefaultResponse();
  }
}

}  // namespace vts
}  // namespace android
