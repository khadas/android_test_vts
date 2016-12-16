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
#include <android/hardware/sensors/1.0/ISensors.h>

using ::android::hardware::sensors::V1_0::ISensors;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  static ::android::sp<ISensors> sensors_hal = ISensors::getService("sensors", true);
  if (sensors_hal == nullptr) {
    return 0;
  }

  size_t min_size = sizeof(int32_t) + sizeof(int64_t);
  if (size < min_size) {
    return 0;
  }

  int32_t sensorHandle;
  memcpy(&sensorHandle, data, sizeof(int32_t));
  data += sizeof(int32_t);

  int64_t samplingPeriodNs;
  memcpy(&samplingPeriodNs, data, sizeof(int64_t));

  sensors_hal->setDelay(sensorHandle, samplingPeriodNs);
  return 0;
}
