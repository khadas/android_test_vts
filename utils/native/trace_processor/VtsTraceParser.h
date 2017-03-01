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

#ifndef TOOLS_TRACE_PROCESSOR_VTSTRACEPARSER_H_
#define TOOLS_TRACE_PROCESSOR_VTSTRACEPARSER_H_

#include <android-base/macros.h>
#include <test/vts/proto/VtsProfilingMessage.pb.h>

namespace android {
namespace vts {

class VtsTraceParser {
 public:
  VtsTraceParser() {};
  virtual ~VtsTraceParser() {};

  // Cleanups the given trace file to be used for replaying.
  // Current cleanup includes:
  // 1. remove duplicate trace item (e.g. passthrough event on the server side)
  // 2. remove trace item that could not be replayed (e.g. client event on the
  //    server side).
  void CleanupTraceForReplay(std::string trace_file);
  // Parses the given trace file and outputs the latency for each API call.
  void ProcessTraceForLatencyProfiling(std::string trace_file);

 private:
  // Reads the trace file and parse each trace event into VtsProfilingRecord.
  bool ParseTrace(std::string trace_file,
      std::vector<VtsProfilingRecord>* records);
  // Writes the given list of VtsProfilingRecord into an output file.
  bool WriteRecords(std::string output_file,
      const std::vector<VtsProfilingRecord>& records);

  DISALLOW_COPY_AND_ASSIGN (VtsTraceParser);
};

}  // namespace vts
}  // namespace android
#endif  // TOOLS_TRACE_PROCESSOR_VTSTRACEPARSER_H_
