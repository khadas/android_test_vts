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
#include "VtsTestabilityChecker.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vintf/CompatibilityMatrix.h>
#include <vintf/HalManifest.h>
#include <vintf/parse_xml.h>

using android::vintf::Arch;
using android::vintf::CompatibilityMatrix;
using android::vintf::HalManifest;
using android::vintf::ManifestHal;
using android::vintf::MatrixHal;
using android::vintf::Version;
using android::vintf::XmlConverter;
using android::vintf::gCompatibilityMatrixConverter;
using android::vintf::gHalManifestConverter;
using std::set;
using std::string;

namespace android {
namespace vts {

class VtsTestabilityCheckerTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    test_cm_ = testFrameworkCompMatrix();
    test_fm_ = testFrameworkManfiest();
    test_vm_ = testDeviceManifest();
    checker_.reset(new VtsTestabilityChecker(&test_cm_, &test_fm_, &test_vm_));
  }
  virtual void TearDown() override {}

  HalManifest testDeviceManifest() {
    HalManifest vm;
    string xml =
        "<manifest version=\"1.0\" type=\"framework\">\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.audio</name>\n"
        "        <transport arch=\"32\">passthrough</transport>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IAudio</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.camera</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>1.2</version>\n"
        "        <version>2.5</version>\n"
        "        <interface>\n"
        "            <name>ICamera</name>\n"
        "            <instance>legacy/0</instance>\n"
        "        </interface>\n"
        "        <interface>\n"
        "            <name>IBetterCamera</name>\n"
        "            <instance>camera</instance>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.drm</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IDrm</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.nfc</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>INfc</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.renderscript</name>\n"
        "        <transport arch=\"32+64\">passthrough</transport>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>IRenderscript</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.vibrator</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>IVibrator</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</manifest>\n";
    gHalManifestConverter(&vm, xml);
    return vm;
  }

  HalManifest testFrameworkManfiest() {
    HalManifest fm;
    string xml =
        "<manifest version=\"1.0\" type=\"framework\">\n"
        "    <hal format=\"hidl\">\n"
        "        <name>android.hardware.nfc</name>\n"
        "        <transport>hwbinder</transport>\n"
        "        <version>1.0</version>\n"
        "        <interface>\n"
        "            <name>INfc</name>\n"
        "            <instance>default</instance>\n"
        "            <instance>fnfc</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</manifest>\n";
    gHalManifestConverter(&fm, xml);
    return fm;
  }

  CompatibilityMatrix testFrameworkCompMatrix() {
    CompatibilityMatrix cm;
    string xml =
        "<compatibility-matrix version=\"1.0\" type=\"framework\">\n"
        "    <hal format=\"native\" optional=\"true\">\n"
        "        <name>android.hardware.audio</name>\n"
        "        <version>2.0-1</version>\n"
        "        <interface>\n"
        "            <name>IAudio</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"true\">\n"
        "        <name>android.hardware.camera</name>\n"
        "        <version>2.2-3</version>\n"
        "        <version>4.5-6</version>\n"
        "        <interface>\n"
        "            <name>ICamera</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "        <interface>\n"
        "            <name>IBetterCamera</name>\n"
        "            <instance>camera</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"false\">\n"
        "        <name>android.hardware.drm</name>\n"
        "        <version>1.0-1</version>\n"
        "        <interface>\n"
        "            <name>IDrm</name>\n"
        "            <instance>default</instance>\n"
        "            <instance>drm</instance>\n"
        "        </interface>\n"
        "        <interface>\n"
        "            <name>IDrmTest</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"false\">\n"
        "        <name>android.hardware.light</name>\n"
        "        <version>2.0-1</version>\n"
        "        <interface>\n"
        "            <name>ILight</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"true\">\n"
        "        <name>android.hardware.nfc</name>\n"
        "        <version>1.0-2</version>\n"
        "        <interface>\n"
        "            <name>INfc</name>\n"
        "            <instance>default</instance>\n"
        "            <instance>nfc</instance>\n"
        "        </interface>\n"
        "        <interface>\n"
        "            <name>INfcTest</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"true\">\n"
        "        <name>android.hardware.radio</name>\n"
        "        <version>1.0-1</version>\n"
        "        <interface>\n"
        "            <name>IRadio</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "    <hal format=\"native\" optional=\"false\">\n"
        "        <name>android.hardware.vibrator</name>\n"
        "        <version>2.0</version>\n"
        "        <interface>\n"
        "            <name>IVibrator</name>\n"
        "            <instance>default</instance>\n"
        "        </interface>\n"
        "    </hal>\n"
        "</compatibility-matrix>\n";
    gCompatibilityMatrixConverter(&cm, xml);
    return cm;
  }

 protected:
  CompatibilityMatrix test_cm_;
  HalManifest test_fm_;
  HalManifest test_vm_;
  std::unique_ptr<VtsTestabilityChecker> checker_;
};

TEST_F(VtsTestabilityCheckerTest, CheckComplianceTestForOptionalHal) {
  set<string> instances;
  // Check non-existent hal.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "non-existent", {1, 0}, "None", Arch::ARCH_EMPTY, &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal not required by framework
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.renderscript", {1, 0}, "IRenderscript",
      Arch::ARCH_EMPTY, &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal with unsupported version.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.camera", {1, 0}, "ICamera", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal with non-existent interface.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.camera", {1, 2}, "None", Arch::ARCH_EMPTY, &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal not supported by vendor.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.radio", {1, 0}, "IRadio", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal with version not supported by vendor.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.camera", {4, 5}, "ICamera", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal with interface not supported by vendor.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.nfc", {4, 5}, "INfcTest", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check an option passthrough hal with unsupported arch.
  EXPECT_FALSE(checker_->CheckHalForComplianceTest(
      "android.hardware.audio", {2, 0}, "IAudio", Arch::ARCH_64, &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal supported by vendor but with no compatible
  // instance.
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.camera", {2, 2}, "ICamera", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());

  // Check an optional hal supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.camera", {2, 2}, "IBetterCamera", Arch::ARCH_EMPTY,
      &instances));
  EXPECT_THAT(instances, ::testing::ContainerEq(set<string>({"camera"})));

  // Check an optional hal supported by vendor and framework.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.nfc", {1, 0}, "INfc", Arch::ARCH_EMPTY, &instances));
  EXPECT_THAT(instances,
              ::testing::ContainerEq(set<string>({"default", "fnfc"})));

  // Check an optional passthrough hal supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.audio", {2, 0}, "IAudio", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ::testing::ContainerEq(set<string>({"default"})));

  // Check an optional hal with empty interface (legacy input).
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.camera", {2, 2}, "" /*interface*/, Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());
}

TEST_F(VtsTestabilityCheckerTest, CheckComplianceTestForRequiredHal) {
  set<string> instances;
  // Check a required hal not supported by vendor.
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.light", {2, 0}, "ILight", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ::testing::ContainerEq(set<string>({"default"})));

  // Check a required hal with version not supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest("android.hardware.vibrator",
                                                  {2, 0}, "IVibrator",
                                                  Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ::testing::ContainerEq(set<string>({"default"})));

  // Check a required hal with interface not supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.drm", {1, 0}, "IDrmTest", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances, ::testing::ContainerEq(set<string>({"default"})));

  // Check a required hal supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.drm", {1, 0}, "IDrm", Arch::ARCH_32, &instances));
  EXPECT_THAT(instances,
              ::testing::ContainerEq(set<string>({"default", "drm"})));

  // Check an optional hal with empty interface (legacy input).
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForComplianceTest(
      "android.hardware.vibrator", {2, 0}, "" /*interface*/, Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());
}

TEST_F(VtsTestabilityCheckerTest, CheckNonComplianceTest) {
  set<string> instances;
  // Check non-existent hal.
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "non-existent", {1, 0}, "None", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal with unsupported version by vendor
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.nfc", {2, 0}, "INfc", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());
  // Check hal with unsupported interface by vendor
  EXPECT_FALSE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.nfc", {1, 0}, "INfcTest", Arch::ARCH_32, &instances));
  EXPECT_TRUE(instances.empty());

  // Check hal only supported by vendor
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.renderscript", {1, 0}, "IRenderscript", Arch::ARCH_32,
      &instances));
  EXPECT_THAT(instances, ::testing::ContainerEq(set<string>({"default"})));

  // Check a required hal with version only supported by vendor.
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.vibrator", {1, 0}, "IVibrator", Arch::ARCH_32,
      &instances));
  EXPECT_THAT(instances, ::testing::ContainerEq(set<string>({"default"})));

  // Check hal with additional vendor instance
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.camera", {1, 2}, "IBetterCamera", Arch::ARCH_32,
      &instances));
  EXPECT_THAT(instances,
              ::testing::ContainerEq(set<string>({"default", "camera"})));

  // Check an optional hal with empty interface (legacy input).
  instances.clear();
  EXPECT_TRUE(checker_->CheckHalForNonComplianceTest(
      "android.hardware.vibrator", {1, 0}, "" /*interface*/, Arch::ARCH_EMPTY,
      &instances));
  EXPECT_TRUE(instances.empty());
}

}  // namespace vts
}  // namespace android

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
