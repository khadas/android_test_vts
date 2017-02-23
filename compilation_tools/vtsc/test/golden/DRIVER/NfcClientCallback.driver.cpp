#include "test/vts/specification/hal/NfcClientCallback.vts.h"
#include "vts_measurement.h"
#include <iostream>
#include <hidl/HidlSupport.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include "test/vts/specification/hal/types.vts.h"
#include <android/hidl/base/1.0/types.h>


using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {

::android::hardware::Return<void> Vts_android_hardware_nfc_V1_0_INfcClientCallback::sendEvent(
    ::android::hardware::nfc::V1_0::NfcEvent arg0,
    ::android::hardware::nfc::V1_0::NfcStatus arg1) {
    cout << "sendEvent called" << endl;
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_nfc_V1_0_INfcClientCallback::sendData(
    const ::android::hardware::hidl_vec<uint8_t>& arg0) {
    cout << "sendData called" << endl;
    return ::android::hardware::Void();
}

sp<::android::hardware::nfc::V1_0::INfcClientCallback> VtsFuzzerCreateVts_android_hardware_nfc_V1_0_INfcClientCallback(const string& callback_socket_name) {
    sp<::android::hardware::nfc::V1_0::INfcClientCallback> result;
    result = new Vts_android_hardware_nfc_V1_0_INfcClientCallback();
    return result;
}

}  // namespace vts
}  // namespace android
