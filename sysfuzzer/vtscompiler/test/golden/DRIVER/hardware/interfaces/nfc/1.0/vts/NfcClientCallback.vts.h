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
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include <hidl/HidlSupport.h>



using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {


class VtsNfcClientCallback: public INfcClientCallback {
 public:
    VtsNfcClientCallback() {};

    virtual ~VtsNfcClientCallback() = default;

    ::android::hardware::Return<void> sendEvent(
        ::android::hardware::nfc::V1_0::NfcEvent arg0,
        ::android::hardware::nfc::V1_0::NfcStatus arg1) override;

    ::android::hardware::Return<void> sendData(
        const ::android::hardware::hidl_vec<uint8_t>& arg0) override;

};

VtsNfcClientCallback* VtsFuzzerCreateINfcClientCallback(const string& callback_socket_name);

}  // namespace vts
}  // namespace android
#endif
