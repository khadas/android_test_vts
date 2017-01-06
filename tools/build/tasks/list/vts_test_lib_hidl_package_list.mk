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
  android.hardware.sensors.vts.driver@1.0 \
  android.hardware.thermal.vts.driver@1.0 \
  android.hardware.tv.cec.vts.driver@1.0 \
  android.hardware.tv.input.vts.driver@1.0 \
  android.hardware.vehicle.vts.driver@2.0 \
  android.hardware.vibrator.vts.driver@1.0 \
  android.hardware.vr.vts.driver@1.0 \
  android.hardware.boot@1.0-IBootControl-vts.profiler \
  android.hardware.light@2.0-ILight-vts.profiler \
  android.hardware.memtrack@1.0-IMemtrack-vts.profiler \
  android.hardware.nfc@1.0-INfc-vts.profiler \
  android.hardware.nfc@1.0-INfcClientCallback-vts.profiler \
  android.hardware.power@1.0-IPower-vts.profiler \
  android.hardware.sensors@1.0-ISensors-vts.profiler \
  android.hardware.thermal@1.0-IThermal-vts.profiler \
  android.hardware.tv.cec@1.0-IHdmiCec-vts.profiler \
  android.hardware.tv.cec@1.0-IHdmiCecCallback-vts.profiler \
  android.hardware.tv.input@1.0-ITvInput-vts.profiler \
  android.hardware.tv.input@1.0-ITvInputCallback-vts.profiler \
  android.hardware.vehicle@2.0-IVehicle-vts.profiler \
  android.hardware.vehicle@2.0-IVehicleCallback-vts.profiler \
  android.hardware.vibrator@1.0-IVibrator-vts.profiler \
  android.hardware.vr@1.0-IVr-vts.profiler \

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
