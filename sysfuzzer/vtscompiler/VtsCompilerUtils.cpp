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

#include <google/protobuf/text_format.h>

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
    case HAL_CONVENTIONAL_SUBMODULE: return "hal_conventional_submodule";
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


string GetCppVariableType(const std::string scalar_type_string) {
  if (scalar_type_string == "void"
      || scalar_type_string == "bool"
      || scalar_type_string == "int32_t"
      || scalar_type_string == "uint32_t"
      || scalar_type_string == "int8_t"
      || scalar_type_string == "uint8_t"
      || scalar_type_string == "int64_t"
      || scalar_type_string == "uint64_t"
      || scalar_type_string == "int16_t"
      || scalar_type_string == "uint16_t"
      || scalar_type_string == "float"
      || scalar_type_string == "double") {
    return scalar_type_string;
  } else if(scalar_type_string == "ufloat") {
    return "unsigned float";
  } else if(scalar_type_string == "udouble") {
    return "unsigned double";
  } else if (scalar_type_string == "string") {
    return "std::string";
  } else if (scalar_type_string == "pointer") {
    return "void*";
  } else if (scalar_type_string == "char_pointer") {
    return "char*";
  } else if (scalar_type_string == "uchar_pointer") {
    return "unsigned char*";
  } else if (scalar_type_string == "void_pointer") {
    return "void*";
  } else if (scalar_type_string == "function_pointer") {
    return "void*";
  }

  cerr << __func__ << ":" << __LINE__ << " "
      << "error: unknown scalar_type " << scalar_type_string << endl;
  exit(-1);
}


string GetCppVariableType(VariableSpecificationMessage arg) {
  if (arg.type() == TYPE_VOID) {
    return "void";
  } if (arg.type() == TYPE_PREDEFINED) {
    return arg.predefined_type();
  } else if (arg.type() == TYPE_SCALAR) {
    return GetCppVariableType(arg.scalar_type());
  }
  cerr << __func__ << ":" << __LINE__ << " "
      << ": type " << arg.type() << " not supported" << endl;
  string* output = new string();
  google::protobuf::TextFormat::PrintToString(arg, output);
  cerr << *output;
  delete output;
  exit(-1);
}


string GetConversionToProtobufFunctionName(VariableSpecificationMessage arg) {
  if (arg.type() == TYPE_PREDEFINED) {
    if (arg.predefined_type() == "camera_info_t*") {
      return "ConvertCameraInfoToProtobuf";
    } else if (arg.predefined_type() == "hw_device_t**") {
        return "";
    } else {
      cerr << __FILE__ << ":" << __LINE__ << " "
          << "error: unknown instance type " << arg.predefined_type() << endl;
    }
  }
  cerr << __FUNCTION__
      << ": non-supported type " << arg.type() << endl;
  exit(-1);
}


string GetCppInstanceType(VariableSpecificationMessage arg, string msg) {
  if (arg.type() == TYPE_PREDEFINED) {
    if (arg.predefined_type() == "struct light_state_t*") {
      if (msg.length() == 0) {
        return "GenerateLightState()";
      } else {
        return "GenerateLightStateUsingMessage(" + msg + ")";
      }
    } else if (arg.predefined_type() == "GpsCallbacks*") {
      return "GenerateGpsCallbacks()";
    } else if (arg.predefined_type() == "GpsUtcTime") {
      return "GenerateGpsUtcTime()";
    } else if (arg.predefined_type() == "vts_gps_latitude") {
      return "GenerateLatitude()";
    } else if (arg.predefined_type() == "vts_gps_longitude") {
      return "GenerateLongitude()";
    } else if (arg.predefined_type() == "vts_gps_accuracy") {
      return "GenerateGpsAccuracy()";
    } else if (arg.predefined_type() == "vts_gps_flags_uint16") {
      return "GenerateGpsFlagsUint16()";
    } else if (arg.predefined_type() == "GpsPositionMode") {
      return "GenerateGpsPositionMode()";
    } else if (arg.predefined_type() == "GpsPositionRecurrence") {
      return "GenerateGpsPositionRecurrence()";
    } else if (arg.predefined_type() == "hw_module_t*") {
      return "(hw_module_t*) malloc(sizeof(hw_module_t))";
    } else if (arg.predefined_type() == "hw_module_t**") {
      return "(hw_module_t**) malloc(sizeof(hw_module_t*))";
    } else if (arg.predefined_type() == "hw_device_t**") {
      return "(hw_device_t**) malloc(sizeof(hw_device_t*))";
    } else if (arg.predefined_type() == "camera_info_t*") {
      if (msg.length() == 0) {
        return "GenerateCameraInfo()";
      } else {
        return "GenerateCameraInfoUsingMessage(" + msg + ")";
      }
    } else if (arg.predefined_type() == "camera_module_callbacks_t*") {
      return "GenerateCameraModuleCallbacks()";
    } else if (arg.predefined_type() == "camera_notify_callback") {
      return "GenerateCameraNotifyCallback()";
    } else if (arg.predefined_type() == "camera_data_callback") {
      return "GenerateCameraDataCallback()";
    } else if (arg.predefined_type() == "camera_data_timestamp_callback") {
      return "GenerateCameraDataTimestampCallback()";
    } else if (arg.predefined_type() == "camera_request_memory") {
      return "GenerateCameraRequestMemory()";
    } else if (arg.predefined_type() == "wifi_handle*") {
      return "(wifi_handle*) malloc(sizeof(wifi_handle))";
    } else if (arg.predefined_type() == "struct camera_device*") {
      return "(struct camera_device*) malloc(sizeof(struct camera_device))";
    } else if (arg.predefined_type() == "struct preview_stream_ops*") {
      return "(preview_stream_ops*) malloc(sizeof(preview_stream_ops))";
    } else {
      cerr << __FILE__ << ":" << __LINE__ << " "
          << "error: unknown instance type " << arg.predefined_type() << endl;
    }
  } else if (arg.type() == TYPE_SCALAR) {
    if (arg.scalar_type() == "uint32_t") {
      return "RandomUint32()";
    } else if (arg.scalar_type() == "int32_t") {
      return "RandomInt32()";
    } else if (arg.scalar_type() == "uint64_t") {
      return "RandomUint64()";
    } else if (arg.scalar_type() == "int64_t") {
      return "RandomInt64()";
    } else if (arg.scalar_type() == "uint16_t") {
      return "RandomUint16()";
    } else if (arg.scalar_type() == "int16_t") {
      return "RandomInt16()";
    } else if (arg.scalar_type() == "uint8_t") {
      return "RandomUint8()";
    } else if (arg.scalar_type() == "int8_t") {
      return "RandomInt8()";
    } else if (arg.scalar_type() == "char_pointer") {
      return "RandomCharPointer()";
    } else if (arg.scalar_type() == "pointer"
               || arg.scalar_type() == "void_pointer") {
      return "RandomVoidPointer()";
    }
    cerr << __FILE__ << ":" << __LINE__ << " "
        << "error: unsupported scalar data type " << arg.scalar_type() << endl;
    exit(-1);
  }
  cerr << __FUNCTION__ << ": error: unsupported type " << arg.type() << endl;
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
