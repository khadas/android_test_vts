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

#include <stdlib.h>

#include <utils/RefBase.h>
#define LOG_TAG "VtsFuzzerBinderServer"
#include <utils/Log.h>

#include <binder/TextOutput.h>
#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include "binder/VtsFuzzerBinderService.h"
#include "BinderServer.h"

namespace android {
namespace vts {


class BnVtsFuzzer : public BnInterface<IVtsFuzzer> {
  virtual status_t onTransact(
      uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0);
};


status_t BnVtsFuzzer::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
  ALOGD("BnVtsFuzzer::%s(%i) %i", __FUNCTION__, code, flags);

  data.checkInterface(this);
#ifdef VTS_FUZZER_BINDER_DEBUG
  data.print(PLOG);
  endl(PLOG);
#endif

  switch(code) {
    case EXIT:
      Exit();
      break;
    case STATUS: {
      int32_t type = data.readInt32();
      int32_t result = Status(type);

      ALOGD("BnVtsFuzzer::%s status(%i) -> %i",
            __FUNCTION__, type, result);
      if (reply == NULL) {
        ALOGE("reply == NULL");
        abort();
      }
#ifdef VTS_FUZZER_BINDER_DEBUG
      reply->print(PLOG);
      endl(PLOG);
#endif
      reply->writeInt32(result);
      break;
    }
    case CALL: {
      int32_t arg1 = data.readInt32();
      int32_t arg2 = data.readInt32();
      int32_t result = Call(arg1, arg2);

      ALOGD("BnVtsFuzzer::%s call(%i, %i) = %i",
            __FUNCTION__, arg1, arg2, result);
      if (reply == NULL) {
        ALOGE("reply == NULL");
        abort();
      }
#ifdef VTS_FUZZER_BINDER_DEBUG
      reply->print(PLOG);
      endl(PLOG);
#endif
      reply->writeInt32(result);
      break;
    }
    default:
      return BBinder::onTransact(code, data, reply, flags);
  }
  return NO_ERROR;
}


class VtsFuzzerServer : public BnVtsFuzzer {
  void Exit() {
    printf("VtsFuzzerServer::Exit\n");
  }

  int32_t Status(int32_t type) {
    printf("VtsFuzzerServer::Status(%i)\n", type);
    return 0;
  }

  int32_t Call(int32_t arg1, int32_t arg2) {
    printf("VtsFuzzerServer::Call(%i, %i)\n", arg1, arg2);
    return arg1 + arg2;
  }
};


void StartBinderServer() {
  defaultServiceManager()->addService(
      String16(VTS_FUZZER_BINDER_SERVICE_NAME), new VtsFuzzerServer());
  android::ProcessState::self()->startThreadPool();
  IPCThreadState::self()->joinThreadPool();
}

}  // namespace vts
}  // namespace android
