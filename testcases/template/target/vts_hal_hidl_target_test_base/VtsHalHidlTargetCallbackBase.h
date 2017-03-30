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
#include <iostream>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <utility>

using namespace ::std;

constexpr char DEFAULT_CALLBACK_FUNCTION_NAME[] = "";
constexpr chrono::milliseconds DEFAULT_CALLBACK_WAIT_TIMEOUT_INITIAL =
    chrono::minutes(1);

namespace testing {

/*
 * VTS target side test template for callback.
 *
 * Providing wait and notify for callback functionality.
 *
 * A typical usage looks like this:
 *
 * class CallbackArgs {
 *   ArgType1 arg1;
 *   ArgType2 arg2;
 * }
 *
 * class MyCallback
 *     : public ::testing::VtsHalHidlTargetCallbackBase<>,
 *       public CallbackInterface {
 *  public:
 *   CallbackApi1(ArgType1 arg1) {
 *     CallbackArgs data;
 *     data.arg1 = arg1;
 *     NotifyFromCallback("CallbackApi1", data);
 *   }
 *
 *   CallbackApi2(ArgType2 arg2) {
 *     CallbackArgs data;
 *     data.arg1 = arg1;
 *     NotifyFromCallback("CallbackApi2", data);
 *   }
 * }
 *
 * Test(MyTest) {
 *   CallApi1();
 *   CallApi2();
 *   pair<bool, shared_ptr<CallbackArgs>> result;
 *   result = cb_.WaitForCallback("CallbackApi1"); // cb_ as an instance of MyCallback object
 *   EXPECT_TRUE(result.first); // Check wait did not time out
 *   EXPECT_TRUE(result.second); // Check CallbackArgs is received (not nullptr). This is optional.
 *   // Here check value of args using the pointer result.second;
 *   result = cb_.WaitForCallback("CallbackApi2");
 *   EXPECT_TRUE(result.first);
 *   // Here check value of args using the pointer result.second;
 * }
 *
 * Note type of CallbackArgsTemplateClass is same across the class, which means
 * all WaitForCallback method will return the same data type.
 */
template <class CallbackArgsTemplateClass>
class VtsHalHidlTargetCallbackBase {
 public:
  VtsHalHidlTargetCallbackBase()
      : default_wait_timeout(DEFAULT_CALLBACK_WAIT_TIMEOUT_INITIAL) {}

  virtual ~VtsHalHidlTargetCallbackBase() {
    for (auto it : cb_lock_map_) {
      delete it.second;
    }
  }

  // Wait for a callback function in a test.
  // Returns a pair of a boolean and a shared pointer to args data class.
  // Boolean will be false if wait operation timed out. Pointer will be nullptr
  // if no args are pushed from notify function.
  pair<bool, shared_ptr<CallbackArgsTemplateClass>> WaitForCallback(
      const string& callback_function_name = DEFAULT_CALLBACK_FUNCTION_NAME) {
    return WaitForCallback(callback_function_name,
                           GetWaitTimeout(callback_function_name));
  }

  // Wait for a callback function in a test.
  // Returns a pair of a boolean and a shared pointer to args data class.
  // Boolean will be false if wait operation timed out. Pointer will be nullptr
  // if no args are pushed from notify function.
  pair<bool, shared_ptr<CallbackArgsTemplateClass>> WaitForCallback(
      chrono::milliseconds timeout) {
    return WaitForCallback(DEFAULT_CALLBACK_FUNCTION_NAME, timeout);
  }

  // Wait for a callback function in a test.
  // Returns a pair of a boolean and a shared pointer to args data class.
  // Boolean will be false if wait operation timed out. Pointer will be nullptr
  // if no args are pushed from notify function.
  pair<bool, shared_ptr<CallbackArgsTemplateClass>> WaitForCallback(
      const string& callback_function_name, chrono::milliseconds timeout) {
    return GetCallbackLock(callback_function_name)->WaitForCallback(timeout);
  }

  // Notify a waiting test when a callback is invoked.
  void NotifyFromCallback(
      const string& callback_function_name = DEFAULT_CALLBACK_FUNCTION_NAME) {
    GetCallbackLock(callback_function_name)->NotifyFromCallback();
  }

  // Notify a waiting test when a callback is invoked.
  void NotifyFromCallback(const CallbackArgsTemplateClass& data) {
    NotifyFromCallback(DEFAULT_CALLBACK_FUNCTION_NAME, data);
  }

  // Notify a waiting test when a callback is invoked.
  void NotifyFromCallback(const string& callback_function_name,
                          const CallbackArgsTemplateClass& data) {
    GetCallbackLock(callback_function_name)->NotifyFromCallback(data);
  }

  // Clear lock and data for a callback function.
  void ClearForCallback(
      const string& callback_function_name = DEFAULT_CALLBACK_FUNCTION_NAME) {
    GetCallbackLock(callback_function_name, true);
  }

  // Get wait timeout for a specific callback function
  chrono::milliseconds GetWaitTimeout(
      const string& callback_function_name = DEFAULT_CALLBACK_FUNCTION_NAME) {
    unique_lock<mutex> lock(cb_timeout_map_mtx_);
    auto found = cb_timeout_map_.find(callback_function_name);
    if (found == cb_timeout_map_.end()) {
      return default_wait_timeout;
    } else {
      return found->second;
    }
  }

  // Set default wait timeout for a callback function
  void SetWaitTimeoutDefault(chrono::milliseconds timeout) {
    SetWaitTimeout(DEFAULT_CALLBACK_FUNCTION_NAME, timeout);
  }

  // Set default wait timeout for a specific callback function
  void SetWaitTimeout(const string& callback_function_name,
                      chrono::milliseconds timeout) {
    unique_lock<mutex> lock(cb_timeout_map_mtx_);
    auto found = cb_timeout_map_.find(callback_function_name);
    if (found == cb_timeout_map_.end()) {
      cb_timeout_map_.insert({callback_function_name, timeout});
    } else {
      found->second = timeout;
    }
  }

 private:
  class CallbackLock {
   public:
    CallbackLock() : wait_count_(0) {}

    // Wait for a callback function in a test.
    // Returns a pair of a boolean and a shared pointer to args data class.
    // Boolean will be false if wait operation timed out. Pointer will be
    // nullptr if no args are pushed from notify function.
    pair<bool, shared_ptr<CallbackArgsTemplateClass>> WaitForCallback(
        chrono::milliseconds timeout) {
      unique_lock<mutex> lock(wait_mtx_);
      auto expiration = chrono::system_clock::now() + timeout;

      while (wait_count_ == 0) {
        cv_status status = wait_cv_.wait_until(lock, expiration);
        if (status == cv_status::timeout) {
          cerr << "Timed out waiting for callback" << endl;
          return pair<bool, shared_ptr<CallbackArgsTemplateClass>>(false,
                                                                   nullptr);
        }
      }
      wait_count_--;
      unique_lock<mutex> queue_lock(queue_mtx_);
      shared_ptr<CallbackArgsTemplateClass> data;
      if (!arg_data_.empty()) {
        data = arg_data_.front();
        arg_data_.pop();
      }
      return pair<bool, shared_ptr<CallbackArgsTemplateClass>>(true, data);
    }

    // Notify a waiting test when a callback is invoked.
    void NotifyFromCallback() {
      unique_lock<mutex> lock(wait_mtx_);
      Notify();
    }

    // Notify a waiting test when a callback is invoked.
    void NotifyFromCallback(const CallbackArgsTemplateClass& data) {
      unique_lock<mutex> wait_lock(wait_mtx_, defer_lock);
      unique_lock<mutex> queue_lock(queue_mtx_, defer_lock);
      std::lock(wait_lock, queue_lock);

      arg_data_.push(make_shared<CallbackArgsTemplateClass>(data));
      Notify();
    }

   private:
    // Notify a waiting test when a callback is invoked.
    void Notify() {
      wait_count_++;
      wait_cv_.notify_one();
    }

    // Mutex for protecting operations on callback arg data queue
    // Queue mutex should be locked AFTER wait mutex in a function
    mutex queue_mtx_;
    // Mutex for protecting operations on wait count and conditional variable
    mutex wait_mtx_;
    // Conditional variable for callback wait and notify
    condition_variable wait_cv_;
    // Count for callback conditional variable
    unsigned int wait_count_;
    // A queue of callback arg data
    queue<shared_ptr<CallbackArgsTemplateClass>> arg_data_;
  };

  // Get CallbackLock using callback function name.
  CallbackLock* GetCallbackLock(const string& callback_function_name,
                                bool auto_clear = false) {
    unique_lock<mutex> lock(cb_lock_map_mtx_);
    auto found = cb_lock_map_.find(callback_function_name);
    if (found == cb_lock_map_.end()) {
      CallbackLock* result = new CallbackLock();
      cb_lock_map_.insert({callback_function_name, result});
      return result;
    } else {
      if (auto_clear) {
        delete (found->second);
        found->second = new CallbackLock();
      }
      return found->second;
    }
  }

  // A map of function name and CallbackLock object pointers
  unordered_map<string, CallbackLock*> cb_lock_map_;
  // A map of function name and wait timeouts
  unordered_map<string, chrono::milliseconds> cb_timeout_map_;
  // Mutex for protecting operations on function name-CallbackLock pointer map
  mutex cb_lock_map_mtx_;
  // Mutex for protecting operations on function name-wait timeouts map
  mutex cb_timeout_map_mtx_;
  // Default wait timeout
  chrono::milliseconds default_wait_timeout;
};

}  // namespace testing

#endif  // __VTS_HAL_HIDL_TARGET_CALLBACK_BASE_H
