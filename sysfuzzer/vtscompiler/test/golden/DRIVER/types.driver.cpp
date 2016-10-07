#include "hardware/interfaces/nfc/1.0/vts/types.vts.h"
#include "hardware/interfaces/nfc/1.0/vts/types.vts.h"
#include <hidl/HidlSupport.h>
#include <iostream>
#include "vts_datatype.h"
#include "vts_measurement.h"
#include <android/hardware/nfc/1.0/types.h>
#include <android/hardware/nfc/1.0/types.h>
using namespace android::hardware;
using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {



NfcEvent RandomNfcEvent() {
int choice = rand() / 7;
if (choice < 0) choice *= -1;
    if (choice == 0) return NfcEvent::OPEN_CPLT;
    if (choice == 1) return NfcEvent::CLOSE_CPLT;
    if (choice == 2) return NfcEvent::POST_INIT_CPLT;
    if (choice == 3) return NfcEvent::PRE_DISCOVER_CPLT;
    if (choice == 4) return NfcEvent::REQUEST_CONTROL;
    if (choice == 5) return NfcEvent::RELEASE_CONTROL;
    if (choice == 6) return NfcEvent::ERROR;
    return NfcEvent::OPEN_CPLT;
}
NfcStatus RandomNfcStatus() {
int choice = rand() / 5;
if (choice < 0) choice *= -1;
    if (choice == 0) return NfcStatus::OK;
    if (choice == 1) return NfcStatus::FAILED;
    if (choice == 2) return NfcStatus::ERR_TRANSPORT;
    if (choice == 3) return NfcStatus::ERR_CMD_TIMEOUT;
    if (choice == 4) return NfcStatus::REFUSED;
    return NfcStatus::OK;
}
}  // namespace vts
}  // namespace android
