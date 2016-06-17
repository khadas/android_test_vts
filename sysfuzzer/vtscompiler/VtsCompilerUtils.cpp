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

#include "VtsCompilerUtils.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <fstream>

#include "specification_parser/InterfaceSpecificationParser.h"

#include "test/vts/runners/host/proto/InterfaceSpecificationMessage.pb.h"


using namespace std;


namespace android {
namespace vts {

string ComponentClassToString(int component_class) {
  switch(component_class) {
    case UNKNOWN_CLASS: return "unknown_class";
    case HAL_CONVENTIONAL: return "hal_conventional";
    case SHAREDLIB: return "sharedlib";
    case HAL_HIDL: return "hal_hidl";
    case HAL_SUBMODULE: return "hal_submodule";
  }
  cerr << "error: invalid component_class " << component_class << endl;
  exit(-1);
}


string ComponentTypeToString(int component_type) {
  switch(component_type) {
    case UNKNOWN_TYPE: return "unknown_type";
    case AUDIO: return "audio";
    case CAMERA: return "camera";
    case GPS: return "gps";
    case LIGHT: return "light";
  }
  cerr << "error: invalid component_type " << component_type << endl;
  exit(-1);
}


string GetCppVariableType(const std::string primitive_type_string) {
  const char* primitive_type = primitive_type_string.c_str();
  if (!strcmp(primitive_type, "void")
      || !strcmp(primitive_type, "bool")
      || !strcmp(primitive_type, "int32_t")
      || !strcmp(primitive_type, "uint32_t")
      || !strcmp(primitive_type, "int8_t")
      || !strcmp(primitive_type, "uint8_t")
      || !strcmp(primitive_type, "int64_t")
      || !strcmp(primitive_type, "uint64_t")
      || !strcmp(primitive_type, "int16_t")
      || !strcmp(primitive_type, "uint16_t")
      || !strcmp(primitive_type, "float")
      || !strcmp(primitive_type, "double")) {
    return primitive_type_string;
  } else if(!strcmp(primitive_type, "ufloat")) {
    return "unsigned float";
  } else if(!strcmp(primitive_type, "udouble")) {
    return "unsigned double";
  } else if (!strcmp(primitive_type, "string")) {
    return "std::string";
  } else if (!strcmp(primitive_type, "pointer")) {
    return "void*";
  } else if (!strcmp(primitive_type, "char_pointer")) {
    return "char*";
  } else if (!strcmp(primitive_type, "uchar_pointer")) {
    return "unsigned char*";
  } else if (!strcmp(primitive_type, "void_pointer")) {
    return "void*";
  } else if (!strcmp(primitive_type, "function_pointer")) {
    return "void*";
  }

  cerr << __FILE__ << ":" << __LINE__ << " "
      << "error: unknown primitive_type " << primitive_type << endl;
  exit(-1);
}


string GetCppVariableType(ArgumentSpecificationMessage arg) {
  if (arg.aggregate_type().size() == 1) {
    return arg.aggregate_type(0);
  } else if (arg.primitive_type().size() == 1) {
    return GetCppVariableType(arg.primitive_type(0));
  } else if (arg.primitive_type().size() > 1
             || arg.aggregate_type().size() > 1) {
    cerr << __FUNCTION__
        << ": aggregate type with multiple attributes; not supported" << endl;
    exit(-1);
  }
  cerr << __FUNCTION__ << ": error: neither instance nor data type is set" << endl;
  exit(-1);
}


string GetConversionToProtobufFunctionName(ArgumentSpecificationMessage arg) {
  if (arg.aggregate_type().size() == 1) {
    if (!strcmp(arg.aggregate_type(0).c_str(), "camera_info_t*")) {
      return "ConvertCameraInfoToProtobuf";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "hw_device_t**")) {
        return "";
    } else {
      cerr << __FILE__ << ":" << __LINE__ << " "
          << "error: unknown instance type " << arg.aggregate_type(0) << endl;
    }
  } else if (arg.aggregate_type().size() > 1) {
    cerr << __FUNCTION__
        << ": aggregate type with multiple attributes; not supported" << endl;
    exit(-1);
  }
  if (arg.primitive_type().size() > 1) {
    cerr << __FUNCTION__
        << ": a structure with multiple attributes; not supported" << endl;
    exit(-1);
  } else if (arg.primitive_type().size() == 1) {
    cerr << __FILE__ << ":" << __LINE__ << " "
        << "error: unknown data type " << arg.primitive_type(0) << endl;
    exit(-1);
  }
  cerr << __FUNCTION__ << ": error: neither instance nor data type is set" << endl;
  exit(-1);
}


string GetCppInstanceType(ArgumentSpecificationMessage arg, string msg) {
  if (arg.aggregate_type().size() == 1) {
    if (!strcmp(arg.aggregate_type(0).c_str(), "struct light_state_t*")) {
      if (msg.length() == 0) {
        return "GenerateLightState()";
      } else {
        return "GenerateLightStateUsingMessage(" + msg + ")";
      }
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "GpsCallbacks*")) {
      return "GenerateGpsCallbacks()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "GpsUtcTime")) {
      return "GenerateGpsUtcTime()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "vts_gps_latitude")) {
      return "GenerateLatitude()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "vts_gps_longitude")) {
      return "GenerateLongitude()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "vts_gps_accuracy")) {
      return "GenerateGpsAccuracy()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "vts_gps_flags_uint16")) {
      return "GenerateGpsFlagsUint16()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "GpsPositionMode")) {
      return "GenerateGpsPositionMode()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "GpsPositionRecurrence")) {
      return "GenerateGpsPositionRecurrence()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "hw_module_t*")) {
      return "(hw_module_t*) malloc(sizeof(hw_module_t))";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "hw_module_t**")) {
      return "(hw_module_t**) malloc(sizeof(hw_module_t*))";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "hw_device_t**")) {
      return "(hw_device_t**) malloc(sizeof(hw_device_t*))";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "camera_info_t*")) {
      if (msg.length() == 0) {
        return "GenerateCameraInfo()";
      } else {
        return "GenerateCameraInfoUsingMessage(" + msg + ")";
      }
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "camera_module_callbacks_t*")) {
      return "GenerateCameraModuleCallbacks()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "camera_notify_callback")) {
      return "GenerateCameraNotifyCallback()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "camera_data_callback")) {
      return "GenerateCameraDataCallback()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "camera_data_timestamp_callback")) {
      return "GenerateCameraDataTimestampCallback()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "camera_request_memory")) {
      return "GenerateCameraRequestMemory()";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "wifi_handle*")) {
      return "(wifi_handle*) malloc(sizeof(wifi_handle))";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "struct camera_device*")) {
      return "(struct camera_device*) malloc(sizeof(struct camera_device))";
    } else if (!strcmp(arg.aggregate_type(0).c_str(), "struct preview_stream_ops*")) {
      return "(preview_stream_ops*) malloc(sizeof(preview_stream_ops))";
    } else {
      cerr << __FILE__ << ":" << __LINE__ << " "
          << "error: unknown instance type " << arg.aggregate_type(0) << endl;
    }
  } else if (arg.aggregate_type().size() > 1) {
    cerr << __FUNCTION__
        << ": aggregate type with multiple attributes; not supported" << endl;
    exit(-1);
  }
  if (arg.primitive_type().size() > 1) {
    cerr << __FUNCTION__
        << ": a structure with multiple attributes; not supported" << endl;
    exit(-1);
  } else if (arg.primitive_type().size() == 1) {
    if (!strcmp(arg.primitive_type(0).c_str(), "uint32_t")) {
      return "RandomUint32()";
    } else if (!strcmp(arg.primitive_type(0).c_str(), "int32_t")) {
      return "RandomInt32()";
    } else if (!strcmp(arg.primitive_type(0).c_str(), "char_pointer")) {
      return "RandomCharPointer()";
    } else if (!strcmp(arg.primitive_type(0).c_str(), "void_pointer")) {
      return "RandomVoidPointer()";
    }
    cerr << __FILE__ << ":" << __LINE__ << " "
        << "error: unknown data type " << arg.primitive_type(0) << endl;
    exit(-1);
  }
  cerr << __FUNCTION__ << ": error: neither instance nor data type is set" << endl;
  exit(-1);
}


int vts_fs_mkdirs(char* file_path, mode_t mode) {
  char* p;

  for (p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
    *p = '\0';
    if (mkdir(file_path, mode) == -1) {
      if (errno != EEXIST) {
        *p = '/';
        return -1;
      }
    }
    *p='/';
  }
  return 0;
}

}  // namespace vts
}  // namespace android
