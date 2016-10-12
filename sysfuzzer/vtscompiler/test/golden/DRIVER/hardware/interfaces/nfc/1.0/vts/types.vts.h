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
#include <android/hardware/nfc/1.0/types.h>
#include <hidl/HidlSupport.h>
#include <hidl/IServiceManager.h>



using namespace android::hardware;
using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {
nfc_event_t Randomnfc_event_t();
nfc_status_t Randomnfc_status_t();
void MessageTonfc_data_t(const VariableSpecificationMessage& var_msg, nfc_data_t* arg);

}  // namespace vts
}  // namespace android
#endif
