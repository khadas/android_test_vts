#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

vts_test_lib_hidl_packages := \
  libhwbinder \
  libhidlbase \
  libhidltransport \
  android.hardware.boot.vts.driver@1.0 \
  android.hardware.light.vts.driver@2.0 \
  android.hardware.memtrack.vts.driver@1.0 \
  android.hardware.nfc.vts.driver@1.0 \
  android.hardware.power.vts.driver@1.0 \
  android.hardware.thermal.vts.driver@1.0 \
  android.hardware.tv.cec.vts.driver@1.0 \
  android.hardware.vehicle.vts.driver@2.0 \
  android.hardware.vibrator.vts.driver@1.0 \
  android.hardware.vr.vts.driver@1.0 \
  libvts_profiler_hidl_boot@1.0 \
  libvts_profiler_hidl_memtrack@1.0 \
  android.hardware.nfc@1.0-INfc-vts.profiler \
  android.hardware.nfc@1.0-INfcClientCallback-vts.profiler \
  libvts_profiler_hidl_power@1.0 \
  libvts_profiler_hidl_sensors@1.0 \
  libvts_profiler_hidl_thermal@1.0 \
  libvts_profiler_hidl_vibrator@1.0 \
  libvts_profiler_hidl_vr@1.0 \
  libvts_profiler_hidl_tv_cec@1.0 \

vts_test_lib_hidl_packages += \
  audio_effect_hidl_hal_test \
  boot_hidl_hal_test \
  light_hidl_hal_test \
  memtrack_hidl_hal_test \
  nfc_hidl_hal_test \
  power_hidl_hal_test \
  sensors_hidl_hal_test \
  thermal_hidl_hal_test \
  vibrator_hidl_hal_test \
  vr_hidl_hal_test \

