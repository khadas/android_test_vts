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

#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>

#include <google/protobuf/text_format.h>

#include "specification_parser/InterfaceSpecificationParser.h"
#include "utils/StringUtil.h"

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

bool endsWith(const string& s, const string& suffix) {
  return s.size() >= suffix.size() && s.rfind(suffix) == (s.size() - suffix.size());
}

string ComponentClassToString(int component_class) {
  switch (component_class) {
    case UNKNOWN_CLASS:
      return "unknown_class";
    case HAL_CONVENTIONAL:
      return "hal_conventional";
    case HAL_CONVENTIONAL_SUBMODULE:
      return "hal_conventional_submodule";
    case HAL_HIDL:
      return "hal_hidl";
    case HAL_HIDL_WRAPPED_CONVENTIONAL:
      return "hal_hidl_wrapped_conventional";
    case LIB_SHARED:
      return "lib_shared";
  }
  cerr << "error: invalid component_class " << component_class << endl;
  exit(-1);
}

string ComponentTypeToString(int component_type) {
  switch (component_type) {
    case UNKNOWN_TYPE:
      return "unknown_type";
    case AUDIO:
      return "audio";
    case CAMERA:
      return "camera";
    case GPS:
      return "gps";
    case LIGHT:
      return "light";
    case WIFI:
      return "wifi";
    case MOBILE:
      return "mobile";
    case BLUETOOTH:
      return "bluetooth";
    case NFC:
      return "nfc";
    case VIBRATOR:
      return "vibrator";
    case BIONIC_LIBM:
      return "bionic_libm";
  }
  cerr << "error: invalid component_type " << component_type << endl;
  exit(-1);
}

string GetCppVariableType(const std::string scalar_type_string) {
  if (scalar_type_string == "void" ||
      scalar_type_string == "int32_t" || scalar_type_string == "uint32_t" ||
      scalar_type_string == "int8_t" || scalar_type_string == "uint8_t" ||
      scalar_type_string == "int64_t" || scalar_type_string == "uint64_t" ||
      scalar_type_string == "int16_t" || scalar_type_string == "uint16_t" ||
      scalar_type_string == "float_t" || scalar_type_string == "double_t") {
    return scalar_type_string;
  } else if (scalar_type_string == "bool_t") {
    return "bool";
  } else if (scalar_type_string == "ufloat") {
    return "unsigned float";
  } else if (scalar_type_string == "udouble") {
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

string GetCppVariableType(const VariableSpecificationMessage& arg,
                          const ComponentSpecificationMessage* message) {
  if (arg.type() == TYPE_VOID) {
    return "void";
  }
  if (arg.type() == TYPE_PREDEFINED) {
    return arg.predefined_type();
  } else if (arg.type() == TYPE_SCALAR) {
    return GetCppVariableType(arg.scalar_type());
  } else if (arg.type() == TYPE_STRING) {
    return "::android::hardware::hidl_string";
  } else if (arg.type() == TYPE_ENUM) {
    cout << __func__ << ":" << __LINE__ << " "
         << arg.has_enum_value() << " " << arg.has_predefined_type() << endl;
    if (!arg.has_enum_value() && arg.has_predefined_type()) {
      if (!message || message->component_class() != HAL_HIDL) {
        return arg.predefined_type();
      } else {
        if (!endsWith(message->component_name(), "Callback")) {
          return arg.predefined_type();
        } else {
          return arg.predefined_type();
        }
      }
    }
  } else if (arg.type() == TYPE_STRUCT) {
    cout << __func__ << ":" << __LINE__ << " "
         << arg.struct_value_size() << " " << arg.has_predefined_type() << endl;
    if (arg.struct_value_size() == 0 && arg.has_predefined_type()) {
      if (!message || message->component_class() != HAL_HIDL) {
        return arg.predefined_type();
      } else {
        return arg.predefined_type();
      }
    }
  } else if (arg.type() == TYPE_VECTOR) {
    if (arg.vector_value(0).type() == TYPE_SCALAR) {
      if (arg.vector_value(0).scalar_type().length() == 0) {
        cerr << __func__ << ":" << __LINE__ << " ERROR scalar_type not set" << endl;
        exit(-1);
      }
      return "::android::hardware::hidl_vec<"
          + arg.vector_value(0).scalar_type() + ">";
    } else if (arg.vector_value(0).type() == TYPE_STRUCT) {
      if (arg.vector_value(0).struct_type().length() > 0) {
        return "::android::hardware::hidl_vec<"
            + arg.vector_value(0).struct_type() + ">";
      } else if (arg.vector_value(0).predefined_type().length() > 0) {
        return "::android::hardware::hidl_vec<"
            + arg.vector_value(0).predefined_type() + ">";
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR struct_type not set" << endl;
        exit(-1);
      }
    } else if (arg.vector_value(0).type() == TYPE_STRING) {
      return "const ::android::hardware::hidl_vec< "
          "::android::hardware::hidl_string> &";
    } else {
      cerr << __func__ << ":" << __LINE__ << " ERROR unsupported type "
           << arg.vector_value(0).type() << endl;
    }
  } else if (arg.type() == TYPE_HIDL_CALLBACK) {
    return arg.predefined_type();
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
  cerr << __FUNCTION__ << ": non-supported type " << arg.type() << endl;
  exit(-1);
}

string GetCppInstanceType(
    const VariableSpecificationMessage& arg,
    const string& msg,
    const ComponentSpecificationMessage* message) {
  switch(arg.type()) {
    case TYPE_PREDEFINED: {
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
      } else if (endsWith(arg.predefined_type(), "*")) {
        // known use cases: bt_callbacks_t
        return "(" + arg.predefined_type() + ") malloc(sizeof("
            + arg.predefined_type().substr(0, arg.predefined_type().size() - 1)
            + "))";
      } else {
        cerr << __func__ << ":" << __LINE__ << " "
             << "error: unknown instance type " << arg.predefined_type() << endl;
      }
      break;
    }
    case TYPE_SCALAR: {
      if (arg.scalar_type() == "bool_t") {
        return "RandomBool()";
      } else if (arg.scalar_type() == "uint32_t") {
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
      } else if (arg.scalar_type() == "float_t") {
        return "RandomFloat()";
      } else if (arg.scalar_type() == "double_t") {
        return "RandomDouble()";
      } else if (arg.scalar_type() == "char_pointer") {
        return "RandomCharPointer()";
      } else if (arg.scalar_type() == "uchar_pointer") {
        return "(unsigned char*) RandomCharPointer()";
      } else if (arg.scalar_type() == "pointer" ||
                 arg.scalar_type() == "void_pointer") {
        return "RandomVoidPointer()";
      }
      cerr << __FILE__ << ":" << __LINE__ << " "
           << "error: unsupported scalar data type " << arg.scalar_type() << endl;
      exit(-1);
    }
    case TYPE_ENUM: {
      if (!arg.has_enum_value() && arg.has_predefined_type()) {
        if (!message || message->component_class() != HAL_HIDL) {
          return "(" + arg.predefined_type() +  ") RandomUint32()";
        } else {
          std::string predefined_type_name = arg.predefined_type();
          ReplaceSubString(predefined_type_name, "::", "__");
          return "Random" + predefined_type_name + "()";
          // TODO: generate a function which can dynamically choose the value.
          /* for (const auto& attribute : message->attribute()) {
            if (attribute.type() == TYPE_ENUM &&
                attribute.name() == arg.predefined_type()) {
              // TODO: pick at runtime
              return message->component_name() + "::"
                  + arg.predefined_type() + "::"
                  + attribute.enum_value().enumerator(0);
            }
          } */
        }
      } else {
        cerr << __func__
             << " ENUM either has enum value or doesn't have predefined type"
             << endl;
        exit(-1);
      }
      break;
    }
    case TYPE_STRING: {
      return "android::hardware::hidl_string(RandomCharPointer())";
    }
    case TYPE_STRUCT: {
      if (arg.struct_value_size() == 0 && arg.has_predefined_type()) {
        return message->component_name() + "::" + arg.predefined_type() +  "()";
      }
      break;
    }
    case TYPE_VECTOR: {  // only for HAL_HIDL
      // TODO: generate code that initializes a local hidl_vec.
      return "";
    }
    case TYPE_HIDL_CALLBACK: {
      return arg.predefined_type() + "()";
    }
    default:
      break;
  }
  cerr << __func__ << ": error: unsupported type " << arg.type() << endl;
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
    *p = '/';
  }
  return 0;
}

#define DEFAULT_FACTOR 10000

string GetVersionString(float version, bool for_macro) {
  std::ostringstream out;
  if (for_macro) {
    out << "V";
  }
  long version_long = version * DEFAULT_FACTOR;
  out << (version_long / DEFAULT_FACTOR);
  if (!for_macro) {
    out << ".";
  } else {
    out << "_";
  }
  version_long -= (version_long / DEFAULT_FACTOR) * DEFAULT_FACTOR;
  bool first = true;
  long factor = DEFAULT_FACTOR / 10;
  while (first || (version_long > 0 && factor > 1)) {
    out << (version_long / factor);
    version_long -= (version_long / factor) * factor;
    factor /= 10;
    first = false;
  }
  return out.str();
}

}  // namespace vts
}  // namespace android
