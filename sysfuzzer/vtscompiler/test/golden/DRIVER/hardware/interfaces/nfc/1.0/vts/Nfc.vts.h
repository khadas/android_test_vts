#ifndef __VTS_SPEC_Nfc.driver__
#define __VTS_SPEC_Nfc.driver__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#define LOG_TAG "FuzzerExtended_INfc"
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
    FuzzerExtended_INfc() : FuzzerBase(HAL_HIDL), hw_binder_proxy_() { }
 protected:
    bool Fuzz(FunctionSpecificationMessage* func_msg,
              void** result, const string& callback_socket_name);
    bool GetAttribute(FunctionSpecificationMessage* func_msg,
              void** result);
    bool GetService();

 private:
    sp<INfc> hw_binder_proxy_;
};

extern "C" {
extern android::vts::FuzzerBase* 
vts_func_4_android_hardware_nfc_1_(
);
}
}  // namespace vts
}  // namespace android
#endif
