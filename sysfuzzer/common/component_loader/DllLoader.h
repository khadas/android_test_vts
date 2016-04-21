/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef __VTS_SYSFUZZER_COMMON_COMPONENTLOADER_DLLLOADER_H__
#define __VTS_SYSFUZZER_COMMON_COMPONENTLOADER_DLLLOADER_H__

#include <stdio.h>

#include "hardware/hardware.h"

namespace android {
namespace vts {

class FuzzerBase;

// Pointer type for a function in a loaded component.
typedef FuzzerBase* (*loader_function)();


// Component loader implementation for a DLL file.
class DllLoader {
 public:
  DllLoader();
  virtual ~DllLoader();

  // Loads a DLL file.
  // Returns a handle (void *) if successful; NULL otherwise.
  void* Load(const char* file_path);

  // Finds and returns hw_device_t data structure from the loaded file
  // (i.e., a HAL).
  struct hw_device_t* GetHWDevice();

  // Finds and returns a requested function defined in the loaded file.
  // Returns NULL if not found.
  loader_function GetLoaderFunction(const char* function_name);

 private:
  // pointer to a handle of the loaded DLL file.
  void* handle_;

  // pointer to the HAL data structure found in the loaded file.
  struct hw_device_t *device_;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMMON_COMPONENTLOADER_DLLLOADER_H__
