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

/*
 * Example usage:
 *  $ fuzzer --class=hal --type=light --version=1.0 /system/lib/hw/lights.gce_x86.so
 *  $ fuzzer --class=hal --type=gps --version=1.0 /system/lib/hw/gps.gce_x86.so
 *  $ fuzzer --class=hal --type=camera --version=1.0 /system/lib/hw/camera.gce_x86.so
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <iostream>

#include "specification_parser/InterfaceSpecificationParser.h"
#include "specification_parser/SpecificationBuilder.h"

using namespace std;
using namespace android;

#define INTERFACE_SPEC_LIB_FILENAME "libvts_interfacespecification.so"

// the default epoch count where an epoch is the time for a fuzz test run
// (e.g., a function call).
static const int kDefaultEpochCount = 100;

// Dumps usage on stderr.
static void usage() {
  fprintf(
      stderr,
      "Usage: fuzzer [options] <target HAL file path>\n"
      "\n"
      "Android fuzzer v0.1.  To fuzz Android system.\n"
      "\n"
      "Options:\n"
      "--help\n"
      "    Show this message.\n"
      "\n"
      "Recording continues until Ctrl-C is hit or the time limit is reached.\n"
      "\n");
}


// Parses command args and kicks things off.
int main(int argc, char* const argv[]) {
  static const struct option longOptions[] = {
    {"help",        no_argument,       NULL, 'h'},
    {"class",       required_argument, NULL, 'c'},
    {"type",        required_argument, NULL, 't'},
    {"version",     required_argument, NULL, 'v'},
    {"epoch_count", optional_argument, NULL, 'e'},
    {"spec_dir",    required_argument, NULL, 's'},
    {NULL,          0,                 NULL, 0}};
  int target_class;
  int target_type;
  float target_version = 1.0;
  int epoch_count = kDefaultEpochCount;
  string spec_dir_path(DEFAULT_SPEC_DIR_PATH);

  while (true) {
    int optionIndex = 0;
    int ic = getopt_long(argc, argv, "", longOptions, &optionIndex);
    if (ic == -1) {
      break;
    }

    switch (ic) {
      case 'h':
        usage();
        return 0;
      case 'c': {
        string target_class_str = string(optarg);
        transform(target_class_str.begin(), target_class_str.end(),
                  target_class_str.begin(), ::tolower);
        if (!strcmp(target_class_str.c_str(), "hal")) {
          target_class = vts::HAL;
        } else {
          target_class = 0;
        }
        break;
      }
      case 't': {
        string target_type_str = string(optarg);
        transform(target_type_str.begin(), target_type_str.end(),
                  target_type_str.begin(), ::tolower);
        if (!strcmp(target_type_str.c_str(), "camera")) {
          target_type = vts::CAMERA;
        } else if (!strcmp(target_type_str.c_str(), "gps")) {
          target_type = vts::GPS;
        } else if (!strcmp(target_type_str.c_str(), "audio")) {
          target_type = vts::AUDIO;
        } else if (!strcmp(target_type_str.c_str(), "light")) {
          target_type = vts::LIGHT;
        } else {
          target_type = 0;
        }
        break;
      }
      case 'v':
        target_version = atof(optarg);
        break;
      case 'e':
        epoch_count = atoi(optarg);
        if (epoch_count <= 0) {
          fprintf(stderr, "epoch_count must be > 0");
          return 2;
        }
        break;
      case 's':
        spec_dir_path = string(optarg);
        break;
      default:
        if (ic != '?') {
          fprintf(stderr, "getopt_long returned unexpected value 0x%x\n", ic);
        }
        return 2;
    }
  }

  if (optind != argc - 1) {
    fprintf(stderr, "Must specify output file (see --help).\n");
    return 2;
  }

  android::vts::SpecificationBuilder spec_builder(
      spec_dir_path, epoch_count);
  cout << "Result: "
      << spec_builder.Process(argv[optind],
                              INTERFACE_SPEC_LIB_FILENAME,
                              target_class,
                              target_type,
                              target_version) << endl;
  return 0;
}
