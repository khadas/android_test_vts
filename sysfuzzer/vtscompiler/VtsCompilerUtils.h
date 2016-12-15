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

#include <string>

#include "test/vts/runners/host/proto/InterfaceSpecificationMessage.pb.h"


using namespace std;


namespace android {
namespace vts {

// Returns the component class name as a string
extern string ComponentClassToString(int component_class);

// Returns the component type name as a string
extern string ComponentTypeToString(int component_type);

// Returns the C/C++ variable type name of a given data type.
extern string GetCppVariableType(const string primitive_type_string);

// Returns the C/C++ basic variable type name of a given argument.
extern string GetCppVariableType(VariableSpecificationMessage arg);

// Get the C/C++ instance type name of an argument.
extern string GetCppInstanceType(VariableSpecificationMessage arg,
                                 string msg = string());

// Returns the name of a function which can convert the given arg to a protobuf.
extern string GetConversionToProtobufFunctionName(VariableSpecificationMessage arg);

// fs_mkdirs for VTS.
extern int vts_fs_mkdirs(char* file_path, mode_t mode);

}  // namespace vts
}  // namespace android
