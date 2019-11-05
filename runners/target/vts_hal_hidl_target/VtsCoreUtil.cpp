/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <log/log.h>
#include <iostream>

#include "VtsCoreUtil.h"

namespace testing {

// Runs "pm list features" and attempts to find the specified feature in its
// output.
bool deviceSupportsFeature(const char* feature) {
  bool hasFeature = false;
  // This is one of the best stable native interface. Calling AIDL directly
  // would be problematic if the binder interface changes.
  FILE* p = popen("/system/bin/pm list features", "re");
  if (p) {
    char* line = NULL;
    size_t len = 0;
    while (getline(&line, &len, p) > 0) {
      if (strstr(line, feature)) {
        hasFeature = true;
        break;
      }
    }
    pclose(p);
  } else {
    __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "popen failed: %d", errno);
    _exit(EXIT_FAILURE);
  }
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Feature %s: %ssupported",
                      feature, hasFeature ? "" : "not ");
  return hasFeature;
}

}  // namespace testing