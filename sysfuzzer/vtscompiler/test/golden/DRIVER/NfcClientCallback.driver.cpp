#include "hardware/interfaces/nfc/1.0/vts/NfcClientCallback.vts.h"
#include <hidl/HidlSupport.h>
#include <iostream>
#include "vts_datatype.h"
#include "vts_measurement.h"
#include <hidl/HidlSupport.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include "hardware/interfaces/nfc/1.0/vts/types.vts.h"
#include <android/hardware/nfc/1.0/types.h>
using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {




::android::hardware::Return<void> VtsNfcClientCallback::sendEvent(
    ::android::hardware::nfc::V1_0::NfcEvent arg0,
    ::android::hardware::nfc::V1_0::NfcStatus arg1) {
    cout << "sendEvent called" << endl;
    return ::android::hardware::Void();
}

::android::hardware::Return<void> VtsNfcClientCallback::sendData(
    const ::android::hardware::hidl_vec<uint8_t>& arg0) {
    cout << "sendData called" << endl;
    return ::android::hardware::Void();
}

VtsNfcClientCallback* VtsFuzzerCreateINfcClientCallback(const string& callback_socket_name) {
    return new VtsNfcClientCallback();
}

}  // namespace vts
}  // namespace android
