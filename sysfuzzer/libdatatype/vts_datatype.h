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
 *
 * GCE version of frameworks/av/cmds/screenrecord. Only the user interface is
 * the same as that tool (commit hash 015a1b8739ef368e3a7926a8686a8c938d51bcc2).
 * The internal implementation is different as no hardware encoder is
 * currently available on GCE virtual machines. The actual encoding operations
 * are controlled by the remoter which is controlled by this tool via a local
 * socket channel.
 */

#ifndef __VTS_DATATYPE_H__
#define __VTS_DATATYPE_H__

#include "hal_gps.h"
#include "hal_light.h"

#define MAX_CHAR_POINTER_LENGTH 100

namespace android {
namespace vts {

extern void RandomNumberGeneratorReset();
extern uint32_t RandomUint32();
extern int32_t RandomInt32();
extern int64_t RandomInt64();
extern bool RandomBool();
extern char* RandomCharPointer();

}  // namespace vts
}  // namespace android

#endif
