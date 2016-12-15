#ifndef __VTS_SPEC_BluetoothHalV1bt_interface_t.driver__
#define __VTS_SPEC_BluetoothHalV1bt_interface_t.driver__

#include <hardware/hardware.h>
#include <hardware/bluetooth.h>



#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#define LOG_TAG "FuzzerExtended_bt_interface_t"
#include <utils/Log.h>
#include <fuzz_tester/FuzzerBase.h>
#include <fuzz_tester/FuzzerCallbackBase.h>



namespace android {
namespace vts {
class FuzzerExtended_bt_interface_t : public FuzzerBase {
 public:
    FuzzerExtended_bt_interface_t() : FuzzerBase(HAL_CONVENTIONAL_SUBMODULE) { }
 protected:
    bool Fuzz(FunctionSpecificationMessage* func_msg,
              void** result, const string& callback_socket_name);
    bool GetAttribute(FunctionSpecificationMessage* func_msg,
              void** result);
    void SetSubModule(bt_interface_t* submodule) {
        submodule_ = submodule;
    }

 private:
    bt_interface_t* submodule_;
};

}  // namespace vts
}  // namespace android
#endif
