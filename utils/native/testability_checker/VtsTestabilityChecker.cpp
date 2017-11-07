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
#define LOG_TAG "VtsTestabilityChecker"

#include "VtsTestabilityChecker.h"

#include <algorithm>
#include <iostream>
#include <set>

using android::vintf::Arch;
using android::vintf::CompatibilityMatrix;
using android::vintf::gArchStrings;
using android::vintf::HalManifest;
using android::vintf::ManifestHal;
using android::vintf::MatrixHal;
using android::vintf::Transport;
using android::vintf::Version;
using std::set;
using std::string;

namespace android {
namespace vts {

bool VtsTestabilityChecker::CheckHalForComplianceTest(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";
  set<string> famework_hal_instances;
  set<string> vendor_hal_instances;
  bool check_framework_hal = CheckFrameworkManifestHal(
      hal_package_name, hal_version, hal_interface_name, arch,
      &famework_hal_instances);
  bool check_vendor_hal_compliance = CheckFrameworkCompatibleHal(
      hal_package_name, hal_version, hal_interface_name, arch,
      &vendor_hal_instances);
  set_union(famework_hal_instances.begin(), famework_hal_instances.end(),
            vendor_hal_instances.begin(), vendor_hal_instances.end(),
            std::inserter(*instances, instances->begin()));
  return check_framework_hal || check_vendor_hal_compliance;
}

bool VtsTestabilityChecker::CheckHalForNonComplianceTest(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";
  return CheckVendorManifestHal(hal_package_name, hal_version,
                                hal_interface_name, arch, instances);
}

bool VtsTestabilityChecker::CheckFrameworkCompatibleHal(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";
  const MatrixHal* matrix_hal =
      framework_comp_matrix_->getHal(hal_package_name, hal_version);

  if (!matrix_hal) {
    LOG(DEBUG) << "Does not find hal " << hal_package_name
               << " in compatibility matrix";
    return false;
  }

  if (!hal_interface_name.empty() &&
      !matrix_hal->hasInterface(hal_interface_name)) {
    LOG(DEBUG) << "MaxtrixHal " << hal_package_name
               << " does not support interface " << hal_interface_name;
    return false;
  }

  const ManifestHal* manifest_hal =
      device_hal_manifest_->getHal(hal_package_name, hal_version);

  if (!manifest_hal || (!hal_interface_name.empty() &&
                        !manifest_hal->hasInterface(hal_interface_name))) {
    if (!matrix_hal->optional) {
      LOG(ERROR) << "Compatibility error. Hal " << hal_package_name
                 << " is required by framework but not supported by vendor";
      // Return true here to indicate the test should run, but expect the test
      // to fail (due to incompatible vendor and framework HAL).
      GetTestInstances(matrix_hal, nullptr /*ManifestHal*/, hal_interface_name,
                       instances);
      return true;
    }
    return false;
  }

  if (manifest_hal->transport() == Transport::PASSTHROUGH &&
      !CheckPassthroughManifestHal(manifest_hal, arch)) {
    LOG(DEBUG) << "ManfestHal " << hal_package_name
               << " is passthrough and does not support arch "
               << gArchStrings.at(static_cast<int>(arch));
    return false;
  }

  GetTestInstances(matrix_hal, manifest_hal, hal_interface_name, instances);
  return true;
}

void VtsTestabilityChecker::GetTestInstances(const MatrixHal* matrix_hal,
                                             const ManifestHal* manifest_hal,
                                             const string& interface_name,
                                             set<string>* instances) {
  CHECK(matrix_hal || manifest_hal)
      << "MatrixHal and ManifestHal should not both be NULL.";
  CHECK(instances) << "instances set should not be NULL.";
  if (interface_name.empty()) {
    LOG(INFO) << "No interface name provided, return empty instances set.";
    return;
  }

  set<string> manifest_hal_instances;
  if (!manifest_hal || (matrix_hal && !matrix_hal->optional)) {
    // Only matrix_hal is provided or matrix_hal is required, get instances
    // from matrix_hal.
    set<string> matrix_hal_instances = matrix_hal->getInstances(interface_name);
    instances->insert(matrix_hal_instances.begin(), matrix_hal_instances.end());
  } else if (!matrix_hal) {
    // Only manifest_hal is provided, get instances from manifest_hal.
    set<string> manifest_hal_instances =
        manifest_hal->getInstances(interface_name);
    instances->insert(manifest_hal_instances.begin(),
                      manifest_hal_instances.end());
  } else {
    // Both matrix_hal and manifest_hal not null && matrix_hal is optional,
    // get intersection of instances from both matrix_hal and manifest_hal.
    set<string> matrix_hal_instances = matrix_hal->getInstances(interface_name);
    set<string> manifest_hal_instances =
        manifest_hal->getInstances(interface_name);
    set_intersection(matrix_hal_instances.begin(), matrix_hal_instances.end(),
                     manifest_hal_instances.begin(),
                     manifest_hal_instances.end(),
                     std::inserter(*instances, instances->begin()));
  }
}

bool VtsTestabilityChecker::CheckPassthroughManifestHal(
    const ManifestHal* manifest_hal, const Arch& arch) {
  CHECK(manifest_hal) << "ManifestHal should not be NULL.";
  switch (arch) {
    case Arch::ARCH_32: {
      if (android::vintf::has32(manifest_hal->transportArch.arch)) {
        return true;
      }
      break;
    }
    case Arch::ARCH_64: {
      if (android::vintf::has64(manifest_hal->transportArch.arch)) {
        return true;
      }
      break;
    }
    default: {
      LOG(ERROR) << "Unexpected arch to check: "
                 << gArchStrings.at(static_cast<int>(arch));
      break;
    }
  }
  return false;
}

bool VtsTestabilityChecker::CheckFrameworkManifestHal(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";
  return CheckManifestHal(
      framework_hal_manifest_->getHal(hal_package_name, hal_version),
      hal_interface_name, arch, instances);
}

bool VtsTestabilityChecker::CheckVendorManifestHal(
    const string& hal_package_name, const Version& hal_version,
    const string& hal_interface_name, const Arch& arch,
    set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";
  return CheckManifestHal(
      device_hal_manifest_->getHal(hal_package_name, hal_version),
      hal_interface_name, arch, instances);
}

bool VtsTestabilityChecker::CheckManifestHal(const ManifestHal* manifest_hal,
                                             const string& hal_interface_name,
                                             const Arch& arch,
                                             set<string>* instances) {
  CHECK(instances) << "instances set should not be NULL.";
  if (!manifest_hal) {
    LOG(DEBUG) << "Does not find hal " << manifest_hal->name
               << " in manifest file";
    return false;
  }
  if (!hal_interface_name.empty() &&
      !manifest_hal->hasInterface(hal_interface_name)) {
    LOG(DEBUG) << "ManifestHal " << manifest_hal->name
               << " does not support interface " << hal_interface_name;
    return false;
  }

  if (manifest_hal->transport() == Transport::PASSTHROUGH &&
      !CheckPassthroughManifestHal(manifest_hal, arch)) {
    LOG(DEBUG) << "ManfestHal " << manifest_hal->name
               << " is passthrough and does not support arch "
               << gArchStrings.at(static_cast<int>(arch));
    return false;
  }
  GetTestInstances(nullptr /*matrix_hal*/, manifest_hal, hal_interface_name,
                   instances);
  return true;
}

}  // namespace vts
}  // namespace android
