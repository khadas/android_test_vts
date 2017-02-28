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

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <google/protobuf/text_format.h>
#include <test/vts/proto/ComponentSpecificationMessage.pb.h>

using namespace std;
using google::protobuf::TextFormat;

namespace android {
namespace vts {

bool VtsTraceParser::ParseTrace(string trace_file,
    vector<VtsProfilingRecord>* records) {
  ifstream in(trace_file, std::ios::in);
  bool new_record = true;
  string record_str, line;

  while (getline(in, line)) {
    // Assume records are separated by '\n'.
    if (line.empty()) {
      new_record = false;
    }
    if (new_record) {
      record_str += line + "\n";
    } else {
      unique_ptr <VtsProfilingRecord> record(new VtsProfilingRecord());
      if (!TextFormat::MergeFromString(record_str, record.get())) {
        cerr << "Can't parse a given record: " << record_str << endl;
        return false;
      }
      records->push_back(*record);
      new_record = true;
      record_str.clear();
    }
  }
  return true;
}

bool VtsTraceParser::WriteRecords(string output_file,
    const std::vector<VtsProfilingRecord>& records) {
  ofstream output = std::ofstream(output_file, std::fstream::out);
  for (const auto& record : records) {
    string record_str;
    if (!TextFormat::PrintToString(record, &record_str)) {
      cerr << "Can't print the message" << endl;
      return false;
    }
    output << record_str << "\n";
  }
  output.close();
  return true;
}

void VtsTraceParser::CleanupTraceForReplay(string trace_file) {
  vector<VtsProfilingRecord> records;
  if (!ParseTrace(trace_file, &records)) {
    cerr << "Failed to parse trace file: " << trace_file << endl;
    return;
  }
  vector<VtsProfilingRecord> clean_records;
  for (const auto& record : records) {
    if (record.event() == InstrumentationEventType::SERVER_API_ENTRY
        || record.event() == InstrumentationEventType::SERVER_API_EXIT) {
      clean_records.push_back(record);
    }
  }
  string tmp_file = trace_file + "_tmp";
  if (!WriteRecords(tmp_file, clean_records)) {
    cerr << "Failed to write new trace file: " << tmp_file << endl;
    return;
  }
  if (rename(tmp_file.c_str(), trace_file.c_str())) {
    cerr << "Failed to replace old trace file: " << trace_file << endl;
    return;
  }
}

void VtsTraceParser::ProcessTraceForLatencyProfiling(string trace_file) {
  std::vector<VtsProfilingRecord> records;
  if (!ParseTrace(trace_file, &records)) {
    cerr << ": Failed to parse trace file: " << trace_file << endl;
    return;
  }
  if (records.empty()) return;
  if (records[0].event() == InstrumentationEventType::PASSTHROUGH_ENTRY
      || records[0].event() == InstrumentationEventType::PASSTHROUGH_EXIT) {
    cout << "hidl_hal_mode:passthrough" << endl;
  } else {
    cout << "hidl_hal_mode:binder" << endl;
  }

  for (size_t i = 0; i < records.size(); i += 2) {
    string api = records[i].func_msg().name();
    int64_t start_timestamp = records[i].timestamp();
    int64_t end_timestamp = records[i + 1].timestamp();
    int64_t latency = end_timestamp - start_timestamp;
    cout << api << ":" << latency << endl;
  }
}

}  // namespace vts
}  // namespace android
