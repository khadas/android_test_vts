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

#include <iostream>

#include "hardware/hardware.h"

using namespace std;

namespace android {
namespace vts {

DllLoader::DllLoader() : handle_(NULL) {}

DllLoader::~DllLoader() {
  if (!handle_) {
    dlclose(handle_);
    handle_ = NULL;
  }
}

void* DllLoader::Load(const char* file_path) {
  if (!file_path) {
    cerr << __func__ << ": file_path is NULL" << endl;
    return NULL;
  }

  // consider using the load mechanism in hardware/libhardware/hardware.c
  handle_ = dlopen(file_path, RTLD_LAZY);
  if (!handle_) {
    cerr << __func__ << ": " << dlerror() << endl;
    cerr << __func__ << ": Can't load a shared library, " << file_path << "."
         << endl;
    return NULL;
  }
  cout << __func__ << ": DLL loaded " << file_path << endl;
  return handle_;
}

loader_function DllLoader::GetLoaderFunction(const char* function_name) const {
  return (loader_function)LoadSymbol(function_name);
}

loader_function_with_arg DllLoader::GetLoaderFunctionWithArg(
    const char* function_name) const {
  return (loader_function_with_arg)LoadSymbol(function_name);
}

bool DllLoader::SancovResetCoverage() {
  void (*func)() = (void (*)())LoadSymbol("__sanitizer_reset_coverage");
  if (func == NULL) {
    return false;
  }
  func();
  return true;
}

bool DllLoader::GcovInit(writeout_fn wfn, flush_fn ffn) {
  void (*func)(writeout_fn, flush_fn) =
      (void (*)(writeout_fn, flush_fn))LoadSymbol("llvm_gcov_init");
  if (func == NULL) {
    return false;
  }
  func(wfn, ffn);
  return true;
}

bool DllLoader::GcovFlush() {
  void (*func)() = (void (*)()) LoadSymbol("__gcov_flush");
  if (func == NULL) {
    return false;
  }
  func();
  return true;
}

void* DllLoader::LoadSymbol(const char* symbol_name) const {
  const char* error = dlerror();
  if (error != NULL) {
    cerr << __func__ << ": existing error message before loading "
         << symbol_name << endl;
    cerr << __func__ << ": " << error << endl;
  }
  void* sym = dlsym(handle_, symbol_name);
  if ((error = dlerror()) != NULL) {
    cerr << __func__ << ": Can't find " << symbol_name << endl;
    cerr << __func__ << ": " << error << endl;
    return NULL;
  }
  return sym;
}

}  // namespace vts
}  // namespace android
