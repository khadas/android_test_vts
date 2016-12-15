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

#ifndef __VTS_FUZZER_BINDER_SERVICE_H__
#define __VTS_FUZZER_BINDER_SERVICE_H__

#include <utils/RefBase.h>

#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>

// Place to print the parcel contents (aout, alog, or aerr).
#define PLOG alog

// #ifdef VTS_FUZZER_BINDER_DEBUG

#define VTS_FUZZER_BINDER_SERVICE_NAME "VtsFuzzer"

namespace android {
namespace vts {

// VTS Fuzzer Binder Interface
class IVtsFuzzer : public IInterface {
 public:
  enum {
    EXIT = IBinder::FIRST_CALL_TRANSACTION,
    STATUS,
    CALL
  };

  // Sends an exit command.
  virtual void Exit() = 0;

  // Requests to return the specified status.
  virtual int32_t Status(int32_t type) = 0;

  // Requests to call the specified function using the provided arguments.
  virtual int32_t Call(int32_t arg1, int32_t arg2) = 0;

  DECLARE_META_INTERFACE(VtsFuzzer);
};


// For client
class BpVtsFuzzer : public BpInterface<IVtsFuzzer> {
 public:
  BpVtsFuzzer(const sp<IBinder>& impl) : BpInterface<IVtsFuzzer>(impl) {}

  void Exit();
  int32_t Status(int32_t type);
  int32_t Call(int32_t arg1, int32_t arg2);
};

}  // namespace vts
}  // namespace android

#endif
