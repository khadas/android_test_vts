/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef __VTS_HAL_HIDL_TARGET_CALLBACK_BASE_H
#define __VTS_HAL_HIDL_TARGET_CALLBACK_BASE_H

#include <chrono>
#include <condition_variable>
#include <mutex>

using namespace ::std;

namespace testing {

// VTS target side test template for callback
class VtsHalHidlTargetCallbackBase {
 public:
  // Wait for a callback function in a test.
  bool waitForCallback(chrono::milliseconds timeout = chrono::minutes(1));

  // Notify a waiting test when a callback is invoked.
  void notifyFromCallback();

  VtsHalHidlTargetCallbackBase() : cb_count_(0) {}

 private:
  // mutex and condition variable for callback wait and notify function.
  mutex cb_mtx_;
  condition_variable cb_cv_;
  unsigned int cb_count_;
};

}  // namespace testing

#endif  // __VTS_HAL_HIDL_TARGET_CALLBACK_BASE_H
