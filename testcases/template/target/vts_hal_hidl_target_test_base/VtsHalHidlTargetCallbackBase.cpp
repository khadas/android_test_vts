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

#include "VtsHalHidlTargetCallbackBase.h"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>

using namespace ::std;

namespace testing {

bool VtsHalHidlTargetCallbackBase::waitForCallback(
    chrono::milliseconds timeout) {
  unique_lock<mutex> lock(cb_mtx_);
  auto expiration = chrono::system_clock::now() + timeout;

  while (cb_count_ == 0) {
    cv_status status = cb_cv_.wait_until(lock, expiration);
    if (status == cv_status::timeout) {
      cerr << "Timed out waiting for callback" << endl;
      return false;
    }
  }
  cb_count_--;
  return true;
}

void VtsHalHidlTargetCallbackBase::notifyFromCallback() {
  unique_lock<mutex> lock(cb_mtx_);
  cb_count_++;
  cb_cv_.notify_one();
}

}  // namespace testing
