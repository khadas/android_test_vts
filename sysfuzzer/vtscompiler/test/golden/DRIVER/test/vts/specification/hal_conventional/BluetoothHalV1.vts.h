#ifndef __VTS_SPEC_BluetoothHalV1.driver__
#define __VTS_SPEC_BluetoothHalV1.driver__

#define LOG_TAG "FuzzerExtended_bluetooth_module_t"
#include <hardware/hardware.h>
#include <hardware/bluetooth.h>


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <fuzz_tester/FuzzerBase.h>
#include <fuzz_tester/FuzzerCallbackBase.h>


namespace android {
namespace vts {
class FuzzerExtended_bluetooth_module_t : public FuzzerBase {
 public:
    FuzzerExtended_bluetooth_module_t() : FuzzerBase(HAL_CONVENTIONAL) {}
 protected:
    bool Fuzz(FunctionSpecificationMessage* func_msg, void** result, const string& callback_socket_name);
    bool CallFunction(FunctionSpecificationMessage* func_msg, void** result, const string& callback_socket_name);
    bool VerifyResults(FunctionSpecificationMessage* func_msg, vector<void *> results);
    bool GetAttribute(FunctionSpecificationMessage* func_msg, void** result);
 private:
};


extern "C" {
extern android::vts::FuzzerBase* vts_func_1_7_1_();
}
}  // namespace vts
}  // namespace android
#endif
