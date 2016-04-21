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

#include "component_loader/DllLoader.h"

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#include <iostream>

#include "hardware/hardware.h"

using namespace std;


namespace android {
namespace vts {


DllLoader::DllLoader()
    : handle_(NULL) {
}


DllLoader::~DllLoader() {
  if (!handle_) {
    dlclose(handle_);
    handle_ = NULL;
  }
}


void* DllLoader::Load(const char* file_path) {
  if (!file_path) {
    cerr << __FUNCTION__ << ": file_path is NULL" << endl;
    return NULL;
  }

  // consider using the load mechanism in hardware/libhardware/hardware.c
  handle_ = dlopen(file_path, RTLD_LAZY);
  if (!handle_) {
    fputs(dlerror(), stderr);
    cerr << "Can't load a dll " << file_path << endl;
    return NULL;
  }
  cout << "DLL loaded " << file_path << endl;
  return handle_;
}


struct hw_device_t* DllLoader::GetHWDevice() {
  if (!handle_) {
    cerr << __FUNCTION__ << ": handle_ is NULL" << endl;
    return NULL;
  }

  struct hw_module_t *hmi = (struct hw_module_t *) dlsym(
      handle_, HAL_MODULE_INFO_SYM_AS_STR);
  if (!hmi) {
    cerr << __FUNCTION__ << ": " << HAL_MODULE_INFO_SYM_AS_STR
        << " not found" << endl;
    return NULL;
  }
  hmi->dso = handle_;
  device_ = NULL;
  hmi->methods->open(hmi, hmi->name, &device_);
  return device_;
}


loader_function DllLoader::GetLoaderFunction(const char* function_name) {
  const char* error;
  loader_function func;

  func = (loader_function) dlsym(handle_, function_name);
  if ((error = dlerror()) != NULL)  {
    fputs(error, stderr);
    cerr << __FUNCTION__ << ": Can't find " << function_name << endl;
    return NULL;
  }
  return func;
}

}  // namespace vts
}  // namespace android
