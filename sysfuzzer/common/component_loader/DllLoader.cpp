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

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "hardware/hardware.h"

using namespace std;

namespace android {
namespace vts {

DllLoader::DllLoader() : handle_(NULL), hmi_(NULL), device_(NULL) {}

DllLoader::~DllLoader() {
  if (!handle_) {
    dlclose(handle_);
    handle_ = NULL;
  }
}

void* DllLoader::Load(const char* file_path, bool is_conventional_hal) {
  if (!file_path) {
    cerr << __FUNCTION__ << ": file_path is NULL" << endl;
    return NULL;
  }

  // consider using the load mechanism in hardware/libhardware/hardware.c
  handle_ = dlopen(file_path, RTLD_LAZY);
  if (!handle_) {
    fputs(dlerror(), stderr);
    cerr << endl
         << "Can't load a shared library, " << file_path << "." << endl;
    return NULL;
  }
  cout << __func__ << " DLL loaded " << file_path << endl;
  if (is_conventional_hal) {
    cout << __func__ << " setting hmi" << endl;
    hmi_ = (struct hw_module_t*)dlsym(handle_, HAL_MODULE_INFO_SYM_AS_STR);
    if (!hmi_) {
      cerr << __func__ << ": " << HAL_MODULE_INFO_SYM_AS_STR << " not found"
           << endl;
    }
  }
  return handle_;
}

struct hw_module_t* DllLoader::InitConventionalHal() {
  if (!handle_) {
    cerr << __FUNCTION__ << ": handle_ is NULL" << endl;
    return NULL;
  }
  hmi_ = (struct hw_module_t*)dlsym(handle_, HAL_MODULE_INFO_SYM_AS_STR);
  if (!hmi_) {
    cerr << __FUNCTION__ << ": " << HAL_MODULE_INFO_SYM_AS_STR << " not found"
         << endl;
    return NULL;
  }
  cout << __FUNCTION__ << ":" << __LINE__ << endl;
  hmi_->dso = handle_;
  device_ = NULL;
  cout << __FUNCTION__ << ": version " << hmi_->module_api_version << endl;
  return hmi_;
}

struct hw_device_t* DllLoader::OpenConventionalHal(const char* module_name) {
  cout << __func__ << endl;
  if (!handle_) {
    cerr << __FUNCTION__ << ": handle_ is NULL" << endl;
    return NULL;
  }
  if (!hmi_) {
    cerr << __FUNCTION__ << ": hmi_ is NULL" << endl;
    return NULL;
  }

  device_ = NULL;
  int ret;
  if (module_name && strlen(module_name) > 0) {
    cout << __FUNCTION__ << ":" << __LINE__ << ": module_name |" << module_name
         << "|" << endl;
    ret =
        hmi_->methods->open(hmi_, module_name, (struct hw_device_t**)&device_);
  } else {
    cout << __FUNCTION__ << ":" << __LINE__ << ": (default) " << hmi_->name
         << endl;
    ret = hmi_->methods->open(hmi_, hmi_->name, (struct hw_device_t**)&device_);
  }
  if (ret != 0) {
    cout << "returns " << ret << " " << strerror(errno) << endl;
  }
  cout << __FUNCTION__ << ":" << __LINE__ << " device_ " << device_ << endl;
  return device_;
}

loader_function DllLoader::GetLoaderFunction(const char* function_name) {
  const char* error;
  loader_function func;

  func = (loader_function)dlsym(handle_, function_name);
  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    cerr << endl;
    cerr << __func__ << ": Can't find " << function_name << endl;
    return NULL;
  }
  return func;
}

bool DllLoader::SancovResetCoverage() {
  const char* error;
  void (*func)();

  func = (void (*)())dlsym(handle_, "__sanitizer_reset_coverage");
  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    cerr << __FUNCTION__ << ": Can't find __sanitizer_reset_coverage" << endl;
    return false;
  }
  func();
  return true;
}

bool DllLoader::GcovInit(writeout_fn wfn, flush_fn ffn) {
  const char* error;
  void (*func)(writeout_fn, flush_fn);

  func = (void (*)(writeout_fn, flush_fn))dlsym(handle_, "llvm_gcov_init");
  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    cerr << __FUNCTION__ << ": Can't find llvm_gcov_init" << endl;
    return false;
  }
  func(wfn, ffn);
  return true;
}

bool DllLoader::GcovFlush() {
  const char* error;
  void (*func)();

  func = (void (*)())dlsym(handle_, "__gcov_flush");
  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    cerr << __FUNCTION__ << ": Can't find __gcov_flush" << endl;
    return false;
  }
  func();

  return true;
}

}  // namespace vts
}  // namespace android
