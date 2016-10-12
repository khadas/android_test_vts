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



nfc_event_t Randomnfc_event_t() {
int choice = rand() / 7;
if (choice < 0) choice *= -1;
    if (choice == 0) return nfc_event_t::HAL_NFC_OPEN_CPLT_EVT;
    if (choice == 1) return nfc_event_t::HAL_NFC_CLOSE_CPLT_EVT;
    if (choice == 2) return nfc_event_t::HAL_NFC_POST_INIT_CPLT_EVT;
    if (choice == 3) return nfc_event_t::HAL_NFC_PRE_DISCOVER_CPLT_EVT;
    if (choice == 4) return nfc_event_t::HAL_NFC_REQUEST_CONTROL_EVT;
    if (choice == 5) return nfc_event_t::HAL_NFC_RELEASE_CONTROL_EVT;
    if (choice == 6) return nfc_event_t::HAL_NFC_ERROR_EVT;
    return nfc_event_t::HAL_NFC_OPEN_CPLT_EVT;
}
nfc_status_t Randomnfc_status_t() {
int choice = rand() / 5;
if (choice < 0) choice *= -1;
    if (choice == 0) return nfc_status_t::HAL_NFC_STATUS_OK;
    if (choice == 1) return nfc_status_t::HAL_NFC_STATUS_FAILED;
    if (choice == 2) return nfc_status_t::HAL_NFC_STATUS_ERR_TRANSPORT;
    if (choice == 3) return nfc_status_t::HAL_NFC_STATUS_ERR_CMD_TIMEOUT;
    if (choice == 4) return nfc_status_t::HAL_NFC_STATUS_REFUSED;
    return nfc_status_t::HAL_NFC_STATUS_OK;
}
void MessageTonfc_data_t(const VariableSpecificationMessage& var_msg, nfc_data_t* arg) {
  arg->data.resize(var_msg.struct_value(0).vector_size());
  for (int value_index = 0; value_index < var_msg.struct_value(0).vector_size(); value_index++) {
    arg->data[value_index] = var_msg.struct_value(0).vector_value(value_index).scalar_value().uint8_t();
  }
}
}  // namespace vts
}  // namespace android
