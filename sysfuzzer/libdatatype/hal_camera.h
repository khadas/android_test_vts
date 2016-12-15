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

#ifndef __vts_libdatatype_hal_camera_h__
#define __vts_libdatatype_hal_camera_h__

#include <hardware/hardware.h>
#include <hardware/camera_common.h>

namespace android {
namespace vts {

// Generates a camera_module_callbacks data structure.
extern camera_module_callbacks_t* GenerateCameraModuleCallbacks();

// Generates a camera_info data structure.
extern camera_info_t* GenerateCameraInfo();

}  // namespace vts
}  // namespace android

#endif
