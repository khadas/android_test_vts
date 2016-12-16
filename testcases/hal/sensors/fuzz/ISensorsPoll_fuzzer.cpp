/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include <algorithm>

#include <FuzzerInterface.h>
#include <android/hardware/sensors/1.0/ISensors.h>

using ::android::hardware::sensors::V1_0::ISensors;

auto _hidl_cb = [](auto x, auto y, auto z){};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  static ::android::sp<ISensors> sensors_hal = ISensors::getService("sensors", true);
  if (sensors_hal == nullptr) {
    return 0;
  }
  if (size < sizeof(int32_t)) {
    return 0;
  }
  int32_t maxCount;
  memcpy(&maxCount, data, sizeof(int32_t));

  sensors_hal->poll(maxCount, _hidl_cb);
  return 0;
}
