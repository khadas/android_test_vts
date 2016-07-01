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

#ifndef __VTS_SHELL_DRIVER_H_
#define __VTS_SHELL_DRIVER_H_

#include <iostream>
#include <string>

#include <VtsDriverCommUtil.h>
#include "test/vts/proto/VtsDriverControlMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

class VtsShellDriver {
 public:
  VtsShellDriver()
      : socket_address_(NULL) {}

  explicit VtsShellDriver(char* socket_address)
      : socket_address_(socket_address) {}

  ~VtsShellDriver() {
    cout << __func__ << endl;
    if (this->socket_address_ != NULL) Close();
  }

  // closes the sockets.
  int Close();

  // start shell driver server on unix socket
  int StartListen();

 private:
  // socket address
  char* socket_address_;

  /*
   * execute a given shell command and return the output file descriptor
   * Please remember to call close_output after usage.
   */
  int ExecShellCommand(const string& command,
                       VtsDriverControlResponseMessage* respond_message);

  /*
   * Handles a socket connection. Will execute a received shell command
   * and send back the output text.
   */
  int HandleShellCommandConnection(int connection_fd);
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SHELL_DRIVER_H_
