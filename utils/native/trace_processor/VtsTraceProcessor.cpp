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

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <google/protobuf/text_format.h>
#include <test/vts/proto/ComponentSpecificationMessage.pb.h>
#include <test/vts/proto/VtsReportMessage.pb.h>
#include "VtsTraceProcessor.h"

using namespace std;
using google::protobuf::TextFormat;

namespace android {
namespace vts {

bool VtsTraceProcessor::ParseTrace(const string& trace_file,
    bool ignore_timestamp, bool entry_only,
    VtsProfilingMessage* profiling_msg) {
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
      VtsProfilingRecord record;
      if (!TextFormat::MergeFromString(record_str, &record)) {
        cerr << "Can't parse a given record: " << record_str << endl;
        return false;
      }
      if (ignore_timestamp) {
        record.clear_timestamp();
      }
      if (entry_only) {
        if (record.event() == InstrumentationEventType::SERVER_API_ENTRY
            || record.event() == InstrumentationEventType::CLIENT_API_ENTRY
            || record.event() == InstrumentationEventType::PASSTHROUGH_ENTRY)
          *profiling_msg->add_records() = record;
      } else {
        *profiling_msg->add_records() = record;
      }
      new_record = true;
      record_str.clear();
    }
  }
  return true;
}

bool VtsTraceProcessor::WriteRecords(const string& output_file,
    const std::vector<VtsProfilingRecord>& records) {
  ofstream output = std::ofstream(output_file, std::fstream::out);
  for (const auto& record : records) {
    string record_str;
    if (!TextFormat::PrintToString(record, &record_str)) {
      cerr << __func__ << ": Can't print the message" << endl;
      return false;
    }
    output << record_str << "\n";
  }
  output.close();
  return true;
}

void VtsTraceProcessor::CleanupTraceForReplay(const string& trace_file) {
  VtsProfilingMessage profiling_msg;
  if (!ParseTrace(trace_file, false, false, &profiling_msg)) {
    cerr << __func__ << ": Failed to parse trace file: " << trace_file << endl;
    return;
  }
  vector<VtsProfilingRecord> clean_records;
  for (const auto& record : profiling_msg.records()) {
    if (record.event() == InstrumentationEventType::SERVER_API_ENTRY
        || record.event() == InstrumentationEventType::SERVER_API_EXIT) {
      clean_records.push_back(record);
    }
  }
  string tmp_file = trace_file + "_tmp";
  if (!WriteRecords(tmp_file, clean_records)) {
    cerr << __func__ << ": Failed to write new trace file: " << tmp_file
         << endl;
    return;
  }
  if (rename(tmp_file.c_str(), trace_file.c_str())) {
    cerr << __func__ << ": Failed to replace old trace file: " << trace_file
         << endl;
    return;
  }
}

void VtsTraceProcessor::ProcessTraceForLatencyProfiling(
    const string& trace_file) {
  VtsProfilingMessage profiling_msg;
  if (!ParseTrace(trace_file, false, false, &profiling_msg)) {
    cerr << __func__ << ": Failed to parse trace file: " << trace_file << endl;
    return;
  }
  if (!profiling_msg.records_size()) return;
  if (profiling_msg.records(0).event()
      == InstrumentationEventType::PASSTHROUGH_ENTRY
      || profiling_msg.records(0).event()
          == InstrumentationEventType::PASSTHROUGH_EXIT) {
    cout << "hidl_hal_mode:passthrough" << endl;
  } else {
    cout << "hidl_hal_mode:binder" << endl;
  }

  for (int i = 0; i < profiling_msg.records_size() - 1; i += 2) {
    string api = profiling_msg.records(i).func_msg().name();
    int64_t start_timestamp = profiling_msg.records(i).timestamp();
    int64_t end_timestamp = profiling_msg.records(i + 1).timestamp();
    int64_t latency = end_timestamp - start_timestamp;
    cout << api << ":" << latency << endl;
  }
}

void VtsTraceProcessor::DedupTraces(const string& trace_dir) {
  DIR *dir = opendir(trace_dir.c_str());
  if (dir == 0) {
    cerr << trace_dir << "does not exist." << endl;
    return;
  }
  vector<VtsProfilingMessage> seen_msgs;
  vector<string> duplicate_trace_files;
  struct dirent *file;
  long total_trace_num = 0;
  long duplicat_trace_num = 0;
  while ((file = readdir(dir)) != NULL) {
    if (file->d_type == DT_REG) {
      total_trace_num++;
      string trace_file = trace_dir;
      if (trace_dir.substr(trace_dir.size() - 1) != "/") {
        trace_file += "/";
      }
      trace_file += file->d_name;
      VtsProfilingMessage profiling_msg;
      if (!ParseTrace(trace_file, true, true, &profiling_msg)) {
        cerr << "Failed to parse trace file: " << trace_file << endl;
        return;
      }
      if (!profiling_msg.records_size()) {  // empty trace file
        duplicate_trace_files.push_back(trace_file);
        duplicat_trace_num++;
        continue;
      }
      auto found = find_if(
          seen_msgs.begin(), seen_msgs.end(),
          [&profiling_msg] (const VtsProfilingMessage& seen_msg) {
            std::string str_profiling_msg;
            std::string str_seen_msg;
            profiling_msg.SerializeToString(&str_profiling_msg);
            seen_msg.SerializeToString(&str_seen_msg);
            return (str_profiling_msg == str_seen_msg);
          });
      if (found == seen_msgs.end()) {
        seen_msgs.push_back(profiling_msg);
      } else {
        duplicate_trace_files.push_back(trace_file);
        duplicat_trace_num++;
      }
    }
  }
  for (const string& duplicate_trace : duplicate_trace_files) {
    cout << "deleting duplicate trace file: " << duplicate_trace << endl;
    remove(duplicate_trace.c_str());
  }
  cout << "Num of traces processed: " << total_trace_num << endl;
  cout << "Num of duplicate trace deleted: " << duplicat_trace_num << endl;
  cout << "Duplicate percentage: "
       << float(duplicat_trace_num) / total_trace_num << endl;
}

bool VtsTraceProcessor::ParseCoverageData(const string& coverage_file,
                                          TestReportMessage* report_msg) {
  ifstream in(coverage_file, std::ios::in);
  string msg_str((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  if (!TextFormat::MergeFromString(msg_str, report_msg)) {
    cerr << __func__ << ": Can't parse a given record: " << msg_str << endl;
    return false;
  }
  return true;
}

void VtsTraceProcessor::UpdateCoverageData(
    const CoverageReportMessage& ref_msg,
    CoverageReportMessage* msg_to_be_updated) {
  if (ref_msg.file_path() == msg_to_be_updated->file_path()) {
    for (int line = 0; line < ref_msg.line_coverage_vector_size(); line++) {
      if (line < msg_to_be_updated->line_coverage_vector_size()) {
        if (ref_msg.line_coverage_vector(line) > 0 &&
            msg_to_be_updated->line_coverage_vector(line) > 0) {
          msg_to_be_updated->set_line_coverage_vector(line, 0);
          msg_to_be_updated->set_covered_line_count(
              msg_to_be_updated->covered_line_count() - 1);
        }
      } else {
        cout << "Reached the end of line_coverage_vector." << endl;
        break;
      }
    }
    // sanity check.
    if (msg_to_be_updated->covered_line_count() < 0) {
      cerr << __func__ << ": covered_line_count should not be negative."
           << endl;
      exit(-1);
    }
  }
}

void VtsTraceProcessor::SelectTraces(const string& coverage_file_dir) {
  DIR* dir = opendir(coverage_file_dir.c_str());
  if (dir == 0) {
    cerr << __func__ << ": " << coverage_file_dir << " does not exist." << endl;
    return;
  }
  map<string, TestReportMessage> original_coverage_msgs;
  vector<string> selected_coverage;

  long max_coverage_line = 0;
  string coverage_file_with_max_coverage_line = "";
  struct dirent* file;
  // Parse all the coverage files and store them into original_coverage_msgs.
  while ((file = readdir(dir)) != NULL) {
    if (file->d_type == DT_REG) {
      string coverage_file = coverage_file_dir;
      if (coverage_file_dir.substr(coverage_file_dir.size() - 1) != "/") {
        coverage_file += "/";
      }
      coverage_file += file->d_name;
      TestReportMessage coverage_msg;
      if (!ParseCoverageData(coverage_file, &coverage_msg)) {
        cerr << "Failed to parse coverage file: " << coverage_file << endl;
        return;
      }
      original_coverage_msgs[coverage_file] = coverage_msg;
      long total_coverage_line = GetTotalCoverageLine(coverage_msg);
      cout << "Processed coverage file: " << coverage_file
           << " with total_coverage_line: " << total_coverage_line << endl;
      if (total_coverage_line > max_coverage_line) {
        max_coverage_line = total_coverage_line;
        coverage_file_with_max_coverage_line = coverage_file;
      }
    }
  }
  // Greedy algorithm that selects coverage files with the maximal code coverage
  // delta at each iteration.
  // TODO(zhuoyao): selects the coverage by taking trace file size into
  // consideration. e.g. picks the one with maximum delta/size.
  // Note: Not guaranteed to generate the optimal set.
  // Example (*: covered, -: not_covered)
  // line#\coverage_file   cov1 cov2 cov3
  //          1              *   -    -
  //          2              *   *    -
  //          3              -   *    *
  //          4              -   *    *
  //          5              -   -    *
  // This algorithm will select cov2, cov1, cov3 while optimal solution is:
  // cov1, cov3.
  while (max_coverage_line > 0) {
    selected_coverage.push_back(coverage_file_with_max_coverage_line);
    TestReportMessage selected_coverage_msg =
        original_coverage_msgs[coverage_file_with_max_coverage_line];
    // Remove the coverage file from original_coverage_msgs.
    original_coverage_msgs.erase(coverage_file_with_max_coverage_line);

    max_coverage_line = 0;
    coverage_file_with_max_coverage_line = "";
    // Update the remaining coverage file in original_coverage_msgs.
    for (auto it = original_coverage_msgs.begin();
         it != original_coverage_msgs.end(); ++it) {
      for (const auto ref_coverage : selected_coverage_msg.coverage()) {
        for (int i = 0; i < it->second.coverage_size(); i++) {
          CoverageReportMessage* coverage_to_be_updated =
              it->second.mutable_coverage(i);
          UpdateCoverageData(ref_coverage, coverage_to_be_updated);
        }
      }
      long total_coverage_line = GetTotalCoverageLine(it->second);
      if (total_coverage_line > max_coverage_line) {
        max_coverage_line = total_coverage_line;
        coverage_file_with_max_coverage_line = it->first;
      }
    }
  }
  // TODO(zhuoyao): find out the trace files corresponding to the selected
  // coverage file.
  for (auto coverage : selected_coverage) {
    cout << "select coverage file: " << coverage << endl;
  }
}

long VtsTraceProcessor::GetTotalCoverageLine(const TestReportMessage& msg) {
  long total_coverage_line = 0;
  for (const auto coverage : msg.coverage()) {
    total_coverage_line += coverage.covered_line_count();
  }
  return total_coverage_line;
}

}  // namespace vts
}  // namespace android
