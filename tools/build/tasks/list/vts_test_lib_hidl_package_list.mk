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
  android.hardware.audio.vts.driver@2.0 \
  android.hardware.audio.effect.vts.driver@2.0 \
  android.hardware.biometrics.fingerprint.vts.driver@2.1 \
  android.hardware.bluetooth.vts.driver@1.0 \
  android.hardware.boot.vts.driver@1.0 \
  android.hardware.broadcastradio.vts.driver@1.0 \
  android.hardware.camera.device.vts.driver@1.0 \
  android.hardware.camera.device.vts.driver@3.2 \
  android.hardware.camera.provider.vts.driver@2.4 \
  android.hardware.configstore.vts.driver@1.0 \
  android.hardware.contexthub.vts.driver@1.0 \
  android.hardware.drm.vts.driver@1.0 \
  android.hardware.dumpstate.vts.driver@1.0 \
  android.hardware.evs.vts.driver@1.0 \
  android.hardware.gatekeeper.vts.driver@1.0 \
  android.hardware.gnss.vts.driver@1.0 \
  android.hardware.graphics.allocator.vts.driver@2.0 \
  android.hardware.graphics.composer.vts.driver@2.1 \
  android.hardware.graphics.mapper.vts.driver@2.0 \
  android.hardware.health.vts.driver@1.0 \
  android.hardware.ir.vts.driver@1.0 \
  android.hardware.keymaster.vts.driver@3.0 \
  android.hardware.light.vts.driver@2.0 \
  android.hardware.media.omx.vts.driver@1.0 \
  android.hardware.memtrack.vts.driver@1.0 \
  android.hardware.nfc.vts.driver@1.0 \
  android.hardware.power.vts.driver@1.0 \
  android.hardware.radio.vts.driver@1.0 \
  android.hardware.sensors.vts.driver@1.0 \
  android.hardware.soundtrigger.vts.driver@2.0 \
  android.hardware.thermal.vts.driver@1.0 \
  android.hardware.tv.cec.vts.driver@1.0 \
  android.hardware.tv.input.vts.driver@1.0 \
  android.hardware.usb.vts.driver@1.0 \
  android.hardware.automotive.vehicle.vts.driver@2.0 \
  android.hardware.vibrator.vts.driver@1.0 \
  android.hardware.vr.vts.driver@1.0 \
  android.hardware.wifi.vts.driver@1.0 \
  android.hardware.wifi.supplicant.vts.driver@1.0 \
  android.hardware.audio@2.0-vts.profiler \
  android.hardware.audio.effect@2.0-vts.profiler \
  android.hardware.biometrics.fingerprint@2.1-vts.profiler \
  android.hardware.bluetooth@1.0-vts.profiler \
  android.hardware.boot@1.0-vts.profiler \
  android.hardware.broadcastradio@1.0-vts.profiler \
  android.hardware.camera.device@1.0-vts.profiler \
  android.hardware.camera.device@3.2-vts.profiler \
  android.hardware.camera.provider@2.4-vts.profiler \
  android.hardware.configstore@1.0-vts.profiler \
  android.hardware.contexthub@1.0-vts.profiler \
  android.hardware.drm@1.0-vts.profiler \
  android.hardware.dumpstate@1.0-vts.profiler \
  android.hardware.evs@1.0-vts.profiler \
  android.hardware.gatekeeper@1.0-vts.profiler \
  android.hardware.gnss@1.0-vts.profiler \
  android.hardware.graphics.allocator@2.0-vts.profiler \
  android.hardware.graphics.composer@2.1-vts.profiler \
  android.hardware.graphics.mapper@2.0-vts.profiler \
  android.hardware.health@1.0-vts.profiler \
  android.hardware.ir@1.0-vts.profiler \
  android.hardware.keymaster@3.0-vts.profiler \
  android.hardware.light@2.0-vts.profiler \
  android.hardware.media.omx@1.0-vts.profiler \
  android.hardware.memtrack@1.0-vts.profiler \
  android.hardware.nfc@1.0-vts.profiler \
  android.hardware.power@1.0-vts.profiler \
  android.hardware.radio@1.0-vts.profiler \
  android.hardware.sensors@1.0-vts.profiler \
  android.hardware.soundtrigger@2.0-vts.profiler \
  android.hardware.thermal@1.0-vts.profiler \
  android.hardware.tv.cec@1.0-vts.profiler \
  android.hardware.tv.input@1.0-vts.profiler \
  android.hardware.usb@1.0-vts.profiler \
  android.hardware.automotive.vehicle@2.0-vts.profiler \
  android.hardware.vibrator@1.0-vts.profiler \
  android.hardware.vr@1.0-vts.profiler \
  android.hardware.wifi@1.0-vts.profiler \
  android.hardware.wifi.supplicant@1.0-vts.profiler \

vts_test_lib_hidl_packages += \
  audio_effect_hidl_hal_test \
  bluetooth_hidl_hal_test \
  boot_hidl_hal_test \
  camera_hidl_hal_test \
  contexthub_hidl_hal_test \
  fingerprint_hidl_hal_test \
  graphics_allocator_hidl_hal_test \
  graphics_mapper_hidl_hal_test \
  graphics_composer_hidl_hal_test \
  ir_hidl_hal_test \
  light_hidl_hal_test \
  memtrack_hidl_hal_test \
  gatekeeper_hidl_hal_test \
  nfc_hidl_hal_test \
  power_hidl_hal_test \
  sensors_hidl_hal_test \
  soundtrigger_hidl_hal_test \
  thermal_hidl_hal_test \
  tv_input_hidl_hal_test \
  vibrator_hidl_hal_test \
  vr_hidl_hal_test \
  wifi_hidl_test \
