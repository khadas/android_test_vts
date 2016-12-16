#include "hardware/interfaces/nfc/1.0/vts/types.vts.h"
#include <hidl/HidlSupport.h>
#include <android/hardware/nfc/1.0/types.h>
#include "vts_datatype.h"
#include "vts_measurement.h"
#include <iostream>
using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {



::android::hardware::nfc::V1_0::NfcEvent EnumValue__android__hardware__nfc__V1_0__NfcEvent(const EnumDataValueMessage& arg) {
    return (::android::hardware::nfc::V1_0::NfcEvent) arg.scalar_value(0).uint32_t();
}
::android::hardware::nfc::V1_0::NfcEvent Random__android__hardware__nfc__V1_0__NfcEvent() {
    uint32_t choice = (uint32_t) rand() / 7;
    if (choice == (uint32_t) 0) return ::android::hardware::nfc::V1_0::NfcEvent::OPEN_CPLT;
    if (choice == (uint32_t) 1) return ::android::hardware::nfc::V1_0::NfcEvent::CLOSE_CPLT;
    if (choice == (uint32_t) 2) return ::android::hardware::nfc::V1_0::NfcEvent::POST_INIT_CPLT;
    if (choice == (uint32_t) 3) return ::android::hardware::nfc::V1_0::NfcEvent::PRE_DISCOVER_CPLT;
    if (choice == (uint32_t) 4) return ::android::hardware::nfc::V1_0::NfcEvent::REQUEST_CONTROL;
    if (choice == (uint32_t) 5) return ::android::hardware::nfc::V1_0::NfcEvent::RELEASE_CONTROL;
    if (choice == (uint32_t) 6) return ::android::hardware::nfc::V1_0::NfcEvent::ERROR;
    return ::android::hardware::nfc::V1_0::NfcEvent::OPEN_CPLT;
}
::android::hardware::nfc::V1_0::NfcStatus EnumValue__android__hardware__nfc__V1_0__NfcStatus(const EnumDataValueMessage& arg) {
    return (::android::hardware::nfc::V1_0::NfcStatus) arg.scalar_value(0).uint32_t();
}
::android::hardware::nfc::V1_0::NfcStatus Random__android__hardware__nfc__V1_0__NfcStatus() {
    uint32_t choice = (uint32_t) rand() / 5;
    if (choice == (uint32_t) 0) return ::android::hardware::nfc::V1_0::NfcStatus::OK;
    if (choice == (uint32_t) 1) return ::android::hardware::nfc::V1_0::NfcStatus::FAILED;
    if (choice == (uint32_t) 2) return ::android::hardware::nfc::V1_0::NfcStatus::ERR_TRANSPORT;
    if (choice == (uint32_t) 3) return ::android::hardware::nfc::V1_0::NfcStatus::ERR_CMD_TIMEOUT;
    if (choice == (uint32_t) 4) return ::android::hardware::nfc::V1_0::NfcStatus::REFUSED;
    return ::android::hardware::nfc::V1_0::NfcStatus::OK;
}
}  // namespace vts
}  // namespace android
