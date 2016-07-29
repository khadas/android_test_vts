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

#ifndef __VTS_DRIVER_PROFILING_INTERFACE_H_
#define __VTS_DRIVER_PROFILING_INTERFACE_H_

#include <android-base/macros.h>

#include "test/vts/proto/InterfaceSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

// Library class to trace, record, replay, and profile a HIDL HAL
// implementation.
class VtsProfilingInterface {
 public:
  // for the API entry on the stub side.
  static const int kProfilingPointEntry;
  // for the synchronous callback event on the stub side.
  static const int kProfilingPointCallback;
  // for the API exit on the stub side.
  static const int kProfilingPointExit;

  VtsProfilingInterface() {}

  virtual ~VtsProfilingInterface();

  // returns true if the given message is added to the tracing queue.
  bool AddTraceEvent(const InterfaceSpecificationMessage& message);

 private:
  DISALLOW_COPY_AND_ASSIGN(VtsProfilingInterface);
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_DRIVER_PROFILING_INTERFACE_H_
