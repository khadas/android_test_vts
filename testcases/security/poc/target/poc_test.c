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

#include "poc_test.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct option long_options[] = {
  {"device_model", required_argument, 0, 'd'}
};

static DeviceModel TranslateDeviceModel(const char *name) {
  DeviceModel device_model;
  if (!strcmp("Nexus 5", name)) device_model = NEXUS_5;
  if (!strcmp("Nexus 5X", name)) device_model = NEXUS_5X;
  if (!strcmp("Nexus 6", name)) device_model = NEXUS_6;
  if (!strcmp("Nexus 6P", name)) device_model = NEXUS_6P;
  return device_model;
}

VtsHostInput ParseVtsHostFlags(int argc, char *argv[]) {
  VtsHostInput host_input;
  int opt = 0;
  int index = 0;
  while ((opt = getopt_long_only(argc, argv, "", long_options, &index)) != -1) {
    switch(opt) {
      case 'd':
        host_input.device_model = TranslateDeviceModel(optarg);
        break;
      default:
        printf("Wrong parameters.");
        exit(POC_TEST_FAIL);
    }
  }
  return host_input;
}
