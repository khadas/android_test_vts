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

#include <FuzzerInterface.h>
#include <android/hardware/power/1.0/IPower.h>

using ::android::hardware::power::V1_0::IPower;
using ::android::hardware::power::V1_0::PowerHint;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  static ::android::sp<IPower> power_hal = IPower::getService("power", true);
  if (power_hal == nullptr) {
    return 0;
  }

  size_t min_size = sizeof(PowerHint) + sizeof(int32_t);
  if (size < min_size) {
    return 0;
  }

  PowerHint hint;
  memcpy(&hint, data, sizeof(PowerHint));
  data += sizeof(PowerHint);

  int32_t payload;
  memcpy(&payload, data, sizeof(int32_t));

  power_hal->powerHint(hint, payload);
  return 0;
}
