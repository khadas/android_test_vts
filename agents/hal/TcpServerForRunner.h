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

#ifndef __VTS_FUZZER_TCP_SERVER_H__
#define __VTS_FUZZER_TCP_SERVER_H__

namespace android {
namespace vts {

extern int StartTcpServerForRunner(const char* spec_dir_path,
                                   const char* fuzzer_path32,
                                   const char* fuzzer_path64,
                                   const char* shell_path32,
                                   const char* shell_path64);

}  // namespace vts
}  // namespace android

#endif
