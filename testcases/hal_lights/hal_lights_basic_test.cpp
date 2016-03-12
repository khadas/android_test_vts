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

#include <gtest/gtest.h>

#include <stdio.h>

#include <hardware/hardware.h>
#include <hardware/lights.h>

#include <iostream>

using namespace std;

namespace {

// VTS structural testcase for HAL Lights basic functionalities.
class VtsStructuralTestHalLightsBasicTest : public ::testing::Test {

 protected:
  VtsStructuralTestHalLightsBasicTest() {
#if 0
    // TODO(yim): re-enable
    int rc = hw_get_module_by_class(LIGHTS_HARDWARE_MODULE_ID, NULL, &module_);
    if (rc || !module_) {
      cerr << "could not find any lights HAL module." << endl;
      module_ = NULL;
      return;
    }

    rc = module_->methods->open(
        module_, LIGHT_ID_NOTIFICATIONS,
        reinterpret_cast<struct hw_device_t**>(&device_));
    if (rc || !device_) {
      cerr << "could not open a lights HAL device." << endl;
      module_ = NULL;
      return;
    }
#endif
  }

  virtual ~VtsStructuralTestHalLightsBasicTest() {
  }

  virtual void SetUp() {
    // define operations to execute before running each testcase.
  }

  virtual void TearDown() {
    // define operations to execute after running each testcase.
  }

  // a light device which is open.
  struct light_device_t* device_;

 private:
#if 0
  const struct hw_module_t* module_;
#endif
};

TEST_F(VtsStructuralTestHalLightsBasicTest, example) {
  ASSERT_TRUE(device_);
  struct light_state_t* arg = NULL;
  EXPECT_EQ(0, device_->set_light(device_, arg));
}

}  // namespace
