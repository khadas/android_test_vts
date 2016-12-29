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

vts_spec_file_list := \
  test/vts/specification/hal_conventional/CameraHalV2.vts \
  test/vts/specification/hal_conventional/CameraHalV2hw_device_t.vts \
  test/vts/specification/hal_conventional/CameraHalV3.vts \
  test/vts/specification/hal_conventional/CameraHalV3camera3_device_ops_t.vts \
  test/vts/specification/hal_conventional/GpsHalV1.vts \
  test/vts/specification/hal_conventional/GpsHalV1GpsInterface.vts \
  test/vts/specification/hal_conventional/LightHalV1.vts \
  test/vts/specification/hal_conventional/WifiHalV1.vts \
  test/vts/specification/hal_conventional/BluetoothHalV1.vts \
  test/vts/specification/hal_conventional/BluetoothHalV1bt_interface_t.vts \
  test/vts/specification/lib_bionic/libmV1.vts \
  test/vts/specification/lib_bionic/libcV1.vts \
  test/vts/specification/lib_bionic/libcutilsV1.vts \

vts_spec_file_list += \
  hardware/interfaces/tv/input/1.0/vts/TvInput.vts \
  hardware/interfaces/tv/input/1.0/vts/TvInputCallback.vts \
  hardware/interfaces/tv/input/1.0/vts/types.vts \
  hardware/interfaces/nfc/1.0/vts/Nfc.vts \
  hardware/interfaces/nfc/1.0/vts/NfcClientCallback.vts \
  hardware/interfaces/nfc/1.0/vts/types.vts \
  hardware/interfaces/vibrator/1.0/vts/Vibrator.vts \
  hardware/interfaces/vibrator/1.0/vts/types.vts \
  hardware/interfaces/thermal/1.0/vts/Thermal.vts \
  hardware/interfaces/thermal/1.0/vts/types.vts \
  hardware/interfaces/sensors/1.0/vts/Sensors.vts \
  hardware/interfaces/sensors/1.0/vts/types.vts \
  hardware/interfaces/vr/1.0/vts/Vr.vts \
  hardware/interfaces/tv/cec/1.0/vts/HdmiCec.vts \
  hardware/interfaces/tv/cec/1.0/vts/HdmiCecCallback.vts \
  hardware/interfaces/tv/cec/1.0/vts/types.vts \
