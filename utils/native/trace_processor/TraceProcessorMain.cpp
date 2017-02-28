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

#include "VtsTraceParser.h"
// Usage examples:
//   To cleanup trace, <binary> --cleanup <trace file>
//   To profile, <binary> --profiling <trace file>
// Cleanup trace is used to generate trace for replay test, it will replace the
// old trace file with a new one of the same format (VtsProfilingRecord).
// Profile trace will calculate the latency of each API recorded in the trace
// and print them out with the format api:latency. e.g.
//   open:150231474
//   write:842604
//   coreInitialized:30466722
int main(int argc, char* argv[]) {
  std::string trace_file = "";
  if (argc == 3) {
    trace_file = argv[2];
    android::vts::VtsTraceParser trace_parser;
    if (!strcmp(argv[1],"--cleanup")) {
      trace_parser.CleanupTraceForReplay(trace_file);
    } else if(!strcmp(argv[1], "--profiling")) {
      trace_parser.ProcessTraceForLatencyProfiling(trace_file);
    } else {
      fprintf(stderr, "Invalid argument.\n");
      return -1;
    }
  } else {
    fprintf(stderr, "Invalid argument.\n");
    return -1;
  }
  return 0;
}
