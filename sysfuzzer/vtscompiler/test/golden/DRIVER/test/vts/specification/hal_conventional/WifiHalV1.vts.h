#ifndef __VTS_SPEC__WifiHalV1.driver__
#define __VTS_SPEC__WifiHalV1.driver__

#define LOG_TAG "FuzzerExtended_wifi"
#include <hardware/hardware.h>
#include <hardware_legacy/wifi_hal.h>


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <fuzz_tester/FuzzerBase.h>
#include <fuzz_tester/FuzzerCallbackBase.h>


namespace android {
namespace vts {
class FuzzerExtended_wifi : public FuzzerBase {
 public:
    FuzzerExtended_wifi() : FuzzerBase(HAL_LEGACY) {}
 protected:
    bool Fuzz(FunctionSpecificationMessage* func_msg,
              void** result, const string& callback_socket_name);
    bool GetAttribute(FunctionSpecificationMessage* func_msg,
              void** result);
 private:
};


extern "C" {
extern android::vts::FuzzerBase* vts_func_3_5_1_();
}
}  // namespace vts
}  // namespace android
#endif
