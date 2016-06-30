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

#ifndef __VTS_DRIVERS_SHELL_SHELL_DRIVER_H_
#define __VTS_DRIVERS_SHELL_SHELL_DRIVER_H_

namespace android {
namespace vts {

/*
 * starts driver process and listen on the specified UNIX domain socket.
 */
extern int vts_shell_driver_start(char* socket_address);

}  // namespace vts
}  // namespace android

#endif  // __VTS_DRIVERS_SHELL_SHELL_DRIVER_H_
