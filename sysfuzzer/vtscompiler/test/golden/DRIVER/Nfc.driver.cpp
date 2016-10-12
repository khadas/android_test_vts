#include "hardware/interfaces/nfc/1.0/vts/Nfc.vts.h"
#include "hardware/interfaces/nfc/1.0/vts/types.vts.h"
#include <hidl/HidlSupport.h>
#include <iostream>
#include "vts_datatype.h"
#include "vts_measurement.h"
#include <android/hardware/nfc/1.0/INfc.h>
#include <android/hardware/nfc/1.0/types.h>
#include <android/hardware/nfc/1.0/INfc.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include "hardware/interfaces/nfc/1.0/vts/NfcClientCallback.vts.h"
#include <android/hardware/nfc/1.0/types.h>
#include <android/hardware/nfc/1.0/types.h>
#include <android/hardware/nfc/1.0/types.h>
using namespace android::hardware;
using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {



static void FuzzerExtended_INfcopen_cb_func(int32_t arg) {
  cout << "callback open called" << endl;
}
std::function<void(int32_t)> FuzzerExtended_INfcopen_cb = FuzzerExtended_INfcopen_cb_func;


static void FuzzerExtended_INfcwrite_cb_func(int32_t arg) {
  cout << "callback write called" << endl;
}
std::function<void(int32_t)> FuzzerExtended_INfcwrite_cb = FuzzerExtended_INfcwrite_cb_func;


static void FuzzerExtended_INfccore_initialized_cb_func(int32_t arg) {
  cout << "callback core_initialized called" << endl;
}
std::function<void(int32_t)> FuzzerExtended_INfccore_initialized_cb = FuzzerExtended_INfccore_initialized_cb_func;


static void FuzzerExtended_INfcpre_discover_cb_func(int32_t arg) {
  cout << "callback pre_discover called" << endl;
}
std::function<void(int32_t)> FuzzerExtended_INfcpre_discover_cb = FuzzerExtended_INfcpre_discover_cb_func;


static void FuzzerExtended_INfcclose_cb_func(int32_t arg) {
  cout << "callback close called" << endl;
}
std::function<void(int32_t)> FuzzerExtended_INfcclose_cb = FuzzerExtended_INfcclose_cb_func;


static void FuzzerExtended_INfccontrol_granted_cb_func(int32_t arg) {
  cout << "callback control_granted called" << endl;
}
std::function<void(int32_t)> FuzzerExtended_INfccontrol_granted_cb = FuzzerExtended_INfccontrol_granted_cb_func;


static void FuzzerExtended_INfcpower_cycle_cb_func(int32_t arg) {
  cout << "callback power_cycle called" << endl;
}
std::function<void(int32_t)> FuzzerExtended_INfcpower_cycle_cb = FuzzerExtended_INfcpower_cycle_cb_func;


bool FuzzerExtended_INfc::GetService() {
  static bool initialized = false;
  if (!initialized) {
    cout << "[agent:hal] HIDL getService" << endl;
    hw_binder_proxy_ = INfc::getService("nfc");
    initialized = true;
  }
  return true;
}

bool FuzzerExtended_INfc::Fuzz(
    FunctionSpecificationMessage* func_msg,
    void** result, const string& callback_socket_name) {
  const char* func_name = func_msg->name().c_str();
  cout << "Function: " << __func__ << " " << func_name << endl;
  if (!strcmp(func_name, "open")) {
    sp<INfcClientCallback> arg0(VtsFuzzerCreateINfcClientCallback(callback_socket_name));
    VtsMeasurement vts_measurement;
    vts_measurement.Start();
    cout << "ok. let's call." << endl;
    *result = reinterpret_cast<void*>((int32_t)hw_binder_proxy_->open(
      arg0));
    vector<float>* measured = vts_measurement.Stop();
    cout << "time " << (*measured)[0] << endl;
    cout << "called" << endl;
    return true;
  }
  if (!strcmp(func_name, "write")) {
    nfc_data_t arg0;
    MessageTonfc_data_t(func_msg->arg(0), &arg0);
    VtsMeasurement vts_measurement;
    vts_measurement.Start();
    cout << "ok. let's call." << endl;
    *result = reinterpret_cast<void*>((int32_t)hw_binder_proxy_->write(
      arg0));
    vector<float>* measured = vts_measurement.Stop();
    cout << "time " << (*measured)[0] << endl;
    cout << "called" << endl;
    return true;
  }
  if (!strcmp(func_name, "core_initialized")) {
    uint8_t* arg0buffer = (uint8_t*) malloc(func_msg->arg(0).vector_size() * sizeof(uint8_t));
    hidl_vec<uint8_t> arg0;
    for (int vector_index = 0; vector_index < func_msg->arg(0).vector_size(); vector_index++) {
      arg0buffer[vector_index] = func_msg->arg(0).vector_value(vector_index).scalar_value().uint8_t();
    }
arg0.setToExternal(arg0buffer, func_msg->arg(0).vector_size());
    VtsMeasurement vts_measurement;
    vts_measurement.Start();
    cout << "ok. let's call." << endl;
    *result = reinterpret_cast<void*>((int32_t)hw_binder_proxy_->core_initialized(
      arg0));
    vector<float>* measured = vts_measurement.Stop();
    cout << "time " << (*measured)[0] << endl;
    cout << "called" << endl;
    return true;
  }
  if (!strcmp(func_name, "pre_discover")) {
    VtsMeasurement vts_measurement;
    vts_measurement.Start();
    cout << "ok. let's call." << endl;
    *result = reinterpret_cast<void*>((int32_t)hw_binder_proxy_->pre_discover());
    vector<float>* measured = vts_measurement.Stop();
    cout << "time " << (*measured)[0] << endl;
    cout << "called" << endl;
    return true;
  }
  if (!strcmp(func_name, "close")) {
    VtsMeasurement vts_measurement;
    vts_measurement.Start();
    cout << "ok. let's call." << endl;
    *result = reinterpret_cast<void*>((int32_t)hw_binder_proxy_->close());
    vector<float>* measured = vts_measurement.Stop();
    cout << "time " << (*measured)[0] << endl;
    cout << "called" << endl;
    return true;
  }
  if (!strcmp(func_name, "control_granted")) {
    VtsMeasurement vts_measurement;
    vts_measurement.Start();
    cout << "ok. let's call." << endl;
    *result = reinterpret_cast<void*>((int32_t)hw_binder_proxy_->control_granted());
    vector<float>* measured = vts_measurement.Stop();
    cout << "time " << (*measured)[0] << endl;
    cout << "called" << endl;
    return true;
  }
  if (!strcmp(func_name, "power_cycle")) {
    VtsMeasurement vts_measurement;
    vts_measurement.Start();
    cout << "ok. let's call." << endl;
    *result = reinterpret_cast<void*>((int32_t)hw_binder_proxy_->power_cycle());
    vector<float>* measured = vts_measurement.Stop();
    cout << "time " << (*measured)[0] << endl;
    cout << "called" << endl;
    return true;
  }
  return false;
}
bool FuzzerExtended_INfc::GetAttribute(
    FunctionSpecificationMessage* func_msg,
    void** result) {
  cerr << "attribute not found" << endl;
  return false;
}
extern "C" {
android::vts::FuzzerBase* 
vts_func_4_8_1_(
) {
  return (android::vts::FuzzerBase*) new android::vts::FuzzerExtended_INfc();
}

}
}  // namespace vts
}  // namespace android
