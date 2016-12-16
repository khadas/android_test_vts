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
#include <android/hardware/light/2.0/ILight.h>

using ::android::hardware::light::V2_0::ILight;
using ::android::hardware::light::V2_0::LightState;
using ::android::hardware::light::V2_0::Type;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  static ::android::sp<ILight> light_hal = ILight::getService("light", true);
  if (light_hal == nullptr) {
    return 0;
  }

  if (size < sizeof(Type) + sizeof(LightState)) {
    return 0;
  }

  Type type;
  memcpy(&type, data, sizeof(Type));
  data += sizeof(Type);

  LightState light_state;
  memcpy(&light_state, data, sizeof(LightState));

  light_hal->setLight(type, light_state);
  return 0;
}
