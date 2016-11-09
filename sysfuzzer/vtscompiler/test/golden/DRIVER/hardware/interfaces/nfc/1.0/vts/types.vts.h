#ifndef __VTS_SPEC_types.driver__
#define __VTS_SPEC_types.driver__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#define LOG_TAG "FuzzerExtended_types"
#include <utils/Log.h>
#include <fuzz_tester/FuzzerBase.h>
#include <fuzz_tester/FuzzerCallbackBase.h>
#include <android/hardware/nfc/1.0/types.h>
#include <hidl/HidlSupport.h>



using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {
::android::hardware::nfc::V1_0::NfcEvent Random__android__hardware__nfc__V1_0__NfcEvent();
::android::hardware::nfc::V1_0::NfcStatus Random__android__hardware__nfc__V1_0__NfcStatus();

}  // namespace vts
}  // namespace android
#endif
