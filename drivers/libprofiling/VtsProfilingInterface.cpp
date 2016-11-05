/*
 * Copyright 2016 The Android Open Source Project
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
#include "VtsProfilingInterface.h"

#include <cutils/properties.h>
#include <fstream>
#include <string>

#include <android-base/logging.h>
#include <google/protobuf/text_format.h>

#include "test/vts/proto/VtsDriverControlMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

const int VtsProfilingInterface::kProfilingPointEntry = 1;
const int VtsProfilingInterface::kProfilingPointCallback = 2;
const int VtsProfilingInterface::kProfilingPointExit = 3;

VtsProfilingInterface::VtsProfilingInterface(const string& trace_file_path)
    : trace_file_path_(trace_file_path),
      trace_output_(nullptr),
      initialized_(false) {
}

VtsProfilingInterface::~VtsProfilingInterface() {
  if (trace_output_) {
    trace_output_.close();
  }
}

static int64_t NanoTime() {
  std::chrono::nanoseconds duration(
      std::chrono::steady_clock::now().time_since_epoch());
  return static_cast<int64_t>(duration.count());
}

VtsProfilingInterface& VtsProfilingInterface::getInstance(
    const string& trace_file_path) {
  static VtsProfilingInterface instance(trace_file_path);
  return instance;
}

void VtsProfilingInterface::Init() {
  if (initialized_) return;

  // Attach device info and timestamp for the trace file.
  char build_number[PROPERTY_VALUE_MAX];
  char device_id[PROPERTY_VALUE_MAX];
  char product_name[PROPERTY_VALUE_MAX];
  property_get("ro.build.version.incremental", build_number, "unknown_build");
  property_get("ro.serialno", device_id, "unknown_device");
  property_get("ro.build.product", product_name, "unknown_product");

  string file_path = trace_file_path_ + "_" + string(product_name) + "_"
      + string(device_id) + "_" + string(build_number) + "_"
      + to_string(NanoTime()) + "_" + ".vts.trace";

  LOG(INFO) << "Creating new profiler instance with file path: " << file_path;
  trace_output_ = std::ofstream(file_path, std::fstream::out);
  if (!trace_output_) {
    LOG(ERROR) << "Can not open trace file: " << trace_file_path_;
    exit(1);
  }
  initialized_ = true;
}

bool VtsProfilingInterface::AddTraceEvent(
    android::hardware::HidlInstrumentor::InstrumentationEvent event,
    const char* package, const char* version, const char* interface,
    const FunctionSpecificationMessage& message) {
  if (!initialized_) {
    LOG(ERROR) << "Profiler not initialized. ";
    return false;
  }
  string msg_str;
  if (!google::protobuf::TextFormat::PrintToString(message, &msg_str)) {
    LOG(ERROR) << "Can't print the message";
    return false;
  }

  mutex_.lock();
  // Record the event data with the following format:
  // timestamp,event,package_name,package_version,interface_name,message
  trace_output_ << NanoTime() << "," << event << "," << package << ","
                << version << "," << interface << "," << message.name() << "\n";
  trace_output_ << msg_str << "\n";
  trace_output_.flush();
  mutex_.unlock();

  return true;
}

}  // namespace vts
}  // namespace android
