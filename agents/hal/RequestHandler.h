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

#ifndef __VTS_AGENT_REQUEST_HANDLER_H__
#define __VTS_AGENT_REQUEST_HANDLER_H__

#include <string>
#include <vector>

#include "test/vts/proto/AndroidSystemControlMessage.pb.h"
#include "test/vts/proto/InterfaceSpecificationMessage.pb.h"

#include "TcpClient.h"

using namespace std;
using namespace google::protobuf;

namespace android {
namespace vts {

// Class which contains actual methods to handle the runner requests.
class AgentRequestHandler {
 public:
  AgentRequestHandler()
      : service_name_(),
        driver_client_(NULL) {}

  // handles a new session.
  int StartSession(
      int fd, const char* fuzzer_path32, const char* fuzzer_path64,
      const char* spec_dir_path);

 protected:

  // for the LIST_HAL command
  AndroidSystemControlResponseMessage* ListHals(
      const ::google::protobuf::RepeatedPtrField< ::std::string>& base_paths);

  // for the SET_HOST_INFO command.
  AndroidSystemControlResponseMessage* SetHostInfo(const int callback_port);

  // for the CHECK_DRIVER_SERVICE command
  AndroidSystemControlResponseMessage* CheckDriverService(
      const string& service_name);

  // for the LAUNCH_DRIVER_SERVICE command
  AndroidSystemControlResponseMessage* LaunchDriverService(
      const char* spec_dir_path, const string& service_name,
      const string& file_path, const char* fuzzer_path,
      int target_class, int target_type,
      float target_version, const string& module_name);

  // for the LIST_APIS command
  AndroidSystemControlResponseMessage* ListApis();

  // for the CALL_API command
  AndroidSystemControlResponseMessage* CallApi(const string& call_payload);

  // Returns a default response message.
  AndroidSystemControlResponseMessage* DefaultResponse();

 protected:
  // the currently opened, connected service name.
  string service_name_;
  // the port number of a host-side callback server.
  int callback_port_;
  // the socket client of a launched or connected driver.
  VtsDriverSocketClient* driver_client_;
};

}  // namespace vts
}  // namespace android

#endif
