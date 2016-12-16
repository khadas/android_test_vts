#ifndef __VTS_SPEC_NfcClientCallback.driver__
#define __VTS_SPEC_NfcClientCallback.driver__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#define LOG_TAG "FuzzerExtended_INfcClientCallback"
#include <utils/Log.h>
#include <fuzz_tester/FuzzerBase.h>
#include <fuzz_tester/FuzzerCallbackBase.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include <android/hardware/nfc/1.0/types.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include <hidl/HidlSupport.h>



using namespace android::hardware;
using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {


class VtsNfcClientCallback: public INfcClientCallback {
 public:
  VtsNfcClientCallback() {};

  virtual ~VtsNfcClientCallback() = default;

  Return<void> sendEvent(
    NfcEvent arg0,
    NfcStatus arg1) override;

  Return<void> sendData(
    const hidl_vec<uint8_t>& arg0) override;

};

VtsNfcClientCallback* VtsFuzzerCreateINfcClientCallback(const string& callback_socket_name);

}  // namespace vts
}  // namespace android
#endif
