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

#ifndef __VTS_FUZZER_TCP_CLIENT_H_
#define __VTS_FUZZER_TCP_CLIENT_H_

#include <string>
#include <vector>

#include <VtsDriverCommUtil.h>

using namespace std;

namespace android {
namespace vts {

// Socket client instance for an agent to control a driver.
class VtsDriverSocketClient : public VtsDriverCommUtil {
 public:
  explicit VtsDriverSocketClient() : VtsDriverCommUtil() {}

  // closes the socket.
  void Close();

  // Sends a EXIT request;
  bool Exit();

  // Sends a LOAD_HAL request.
  int32_t LoadHal(const string& file_path, int target_class, int target_type,
                  float target_version, const string& module_name);

  // Sends a LIST_FUNCTIONS request.
  const char* GetFunctions();

  // Sends a CALL_FUNCTION request.
  const char* Call(const string& arg);

  // Sends a GET_STATUS request.
  int32_t Status(int32_t type);

  // Sends a EXECUTE request.
  vector<string>* ExecuteShellCommand(
      const ::google::protobuf::RepeatedPtrField<::std::string> shell_command);
};

// returns the socket port file's path for the given service_name.
extern string GetSocketPortFilePath(const string& service_name);

// returns true if the specified driver is running.
bool IsDriverRunning(const string& service_name, int retry_count);

// creates and returns VtsDriverSocketClient of the given service_name.
extern VtsDriverSocketClient* GetDriverSocketClient(const string& service_name);

}  // namespace vts
}  // namespace android

#endif  // __VTS_FUZZER_TCP_CLIENT_H_
