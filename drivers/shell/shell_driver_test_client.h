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

#ifndef __VTS_DRIVERS_SHELL_DRIVER_TEST_CLIENT_H_
#define __VTS_DRIVERS_SHELL_DRIVER_TEST_CLIENT_H_

/*
 * send a command to the driver on specified UNIX domain socket and print out
 * the outputs from driver.
 */
extern char* vts_shell_driver_test_client_start(char* cmd, char* addr_socket);


#endif  // __VTS_DRIVERS_SHELL_DRIVER_TEST_CLIENT_H_

