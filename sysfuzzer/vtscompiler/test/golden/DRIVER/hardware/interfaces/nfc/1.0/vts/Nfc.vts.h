#ifndef __VTS_SPEC_android_hardware_nfc_Nfc.driver__
#define __VTS_SPEC_android_hardware_nfc_Nfc.driver__

#define LOG_TAG "FuzzerExtended_INfc"


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <fuzz_tester/FuzzerBase.h>
#include <fuzz_tester/FuzzerCallbackBase.h>


#include <android/hardware/nfc/1.0/INfc.h>
#include <android/hardware/nfc/1.0/INfc.h>
#include <hidl/HidlSupport.h>


using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {
class FuzzerExtended_INfc : public FuzzerBase {
 public:
    FuzzerExtended_INfc() : FuzzerBase(HAL_HIDL), hw_binder_proxy_() {}
 protected:
    bool Fuzz(FunctionSpecificationMessage* func_msg, void** result, const string& callback_socket_name);
    bool CallFunction(FunctionSpecificationMessage* func_msg, void** result, const string& callback_socket_name);
    bool VerifyResults(FunctionSpecificationMessage* func_msg, vector<void *> results);
    bool GetAttribute(FunctionSpecificationMessage* func_msg, void** result);
    bool GetService(bool get_stub, const char* service_name);

 private:
    sp<INfc> hw_binder_proxy_;
};


extern "C" {
extern android::vts::FuzzerBase* vts_func_4_android_hardware_nfc_1_();
}
}  // namespace vts
}  // namespace android
#endif
