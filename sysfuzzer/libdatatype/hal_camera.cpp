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

#include "hal_camera.h"

#include <stdlib.h>
#include <pthread.h>

#include <hardware/hardware.h>
#include <hardware/camera_common.h>

#include "test/vts/sysfuzzer/common/proto/InterfaceSpecificationMessage.pb.h"

#include "vts_datatype.h"

namespace android {
namespace vts {

// Callbacks {
static void vts_camera_device_status_change(
    const struct camera_module_callbacks*, int camera_id, int new_status) {}

static void vts_torch_mode_status_change(
    const struct camera_module_callbacks*, const char* camera_id,
    int new_status) {}
// } Callbacks


camera_module_callbacks_t* GenerateCameraModuleCallbacks() {
  if (RandomBool()) {
    return NULL;
  } else {
    camera_module_callbacks_t* callbacks =
        (camera_module_callbacks_t*) malloc(sizeof(camera_module_callbacks_t));
    callbacks->camera_device_status_change = vts_camera_device_status_change;
    callbacks->torch_mode_status_change = vts_torch_mode_status_change;
    return callbacks;
  }
}


camera_info_t* GenerateCameraInfo() {
  if (RandomBool()) {
    return NULL;
  } else {
    camera_info_t* caminfo = (camera_info_t*) malloc(sizeof(camera_info_t));
    caminfo->facing = RandomBool() ? CAMERA_FACING_BACK : CAMERA_FACING_FRONT;
    // support CAMERA_FACING_EXTERNAL if CAMERA_MODULE_API_VERSION_2_4 or above
    caminfo->orientation = RandomBool() ? (RandomBool() ? 0 : 90) : (RandomBool() ? 180 : 270);
    caminfo->device_version = CAMERA_MODULE_API_VERSION_2_1;
    caminfo->static_camera_characteristics = NULL;
    caminfo->resource_cost = 50;  // between 50 and 100.
    caminfo->conflicting_devices = NULL;
    caminfo->conflicting_devices_length = 0;

    return caminfo;
  }
      /**
       * The camera's fixed characteristics, which include all static camera metadata
       * specified in system/media/camera/docs/docs.html. This should be a sorted metadata
       * buffer, and may not be modified or freed by the caller. The pointer should remain
       * valid for the lifetime of the camera module, and values in it may not
       * change after it is returned by get_camera_info().
       *
       * Version information (based on camera_module_t.common.module_api_version):
       *
       *  CAMERA_MODULE_API_VERSION_1_0:
       *
       *    Not valid. Extra characteristics are not available. Do not read this
       *    field.
       *
       *  CAMERA_MODULE_API_VERSION_2_0 or higher:
       *
       *    Valid if device_version >= CAMERA_DEVICE_API_VERSION_2_0. Do not read
       *    otherwise.
       *
      const camera_metadata_t *static_camera_characteristics;
       */

      /**
       * An array of camera device IDs represented as NULL-terminated strings
       * indicating other devices that cannot be simultaneously opened while this
       * camera device is in use.
       *
       * This field is intended to be used to indicate that this camera device
       * is a composite of several other camera devices, or otherwise has
       * hardware dependencies that prohibit simultaneous usage. If there are no
       * dependencies, a NULL may be returned in this field to indicate this.
       *
       * The camera service will never simultaneously open any of the devices
       * in this list while this camera device is open.
       *
       * The strings pointed to in this field will not be cleaned up by the camera
       * service, and must remain while this device is plugged in.
       *
       * Version information (based on camera_module_t.common.module_api_version):
       *
       *  CAMERA_MODULE_API_VERSION_2_3 or lower:
       *
       *    Not valid.  Can be assumed to be NULL.  Do not read this field.
       *
       *  CAMERA_MODULE_API_VERSION_2_4 or higher:
       *
       *    Always valid.
      char** conflicting_devices;
       */

      /**
       * The length of the array given in the conflicting_devices field.
       *
       * Version information (based on camera_module_t.common.module_api_version):
       *
       *  CAMERA_MODULE_API_VERSION_2_3 or lower:
       *
       *    Not valid.  Can be assumed to be 0.  Do not read this field.
       *
       *  CAMERA_MODULE_API_VERSION_2_4 or higher:
       *
       *    Always valid.
      size_t conflicting_devices_length;
       */
}


bool ConvertCameraInfoToProtobuf(camera_info_t* raw,
                                 ArgumentSpecificationMessage* msg) {
  //msg->mutable_primitive_value()->Clear();
  //msg->mutable_aggregate_value()->Clear();
  if (!raw) {
    return false;
  }
  msg->add_primitive_value()->set_int32_t(raw->facing);
  msg->add_primitive_value()->set_int32_t(raw->orientation);
  msg->add_primitive_value()->set_uint32_t(raw->device_version);
  // TODO: update for static_camera_characteristics and others
  // msg.add_primitive_value()->set_int32_t(raw->static_camera_characteristics);
  msg->add_primitive_value()->set_int32_t(raw->resource_cost);
  // TODO: support pointer. conflicting_devices is pointer pointer.
  // msg.add_primitive_value()->set_pointer(raw->conflicting_devices);
  // msg.add_primitive_value()->set_int32_t(raw->conflicting_devices_length);
  msg->add_primitive_value()->set_int32_t(0);
  return true;
}

}  // namespace vts
}  // namespace android
