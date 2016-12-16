#include "test/vts/specification/hal_hidl/Nfc/NfcClientCallback.vts.h"
#include "test/vts/specification/hal_hidl/Nfc/types.vts.h"

using namespace android::hardware;
using namespace android::hardware::nfc::V1_0;

namespace android {
namespace vts {


void HIDL_INSTRUMENTATION_FUNCTION(
        InstrumentationEvent event,
        const char* package,
        const char* version,
        const char* interface,
        const char* method,
        std::vector<void *> *args) {
    if (strcmp(package, "android.hardware.nfc") != 0) {
        LOG(WARNING) << "incorrect package.";
        return;
    }
    if (strcmp(version, "1.0") != 0) {
        LOG(WARNING) << "incorrect version.";
        return;
    }
    if (strcmp(interface, "INfcClientCallback") != 0) {
        LOG(WARNING) << "incorrect interface.";
        return;
    }
    if (strcmp(method, "sendEvent") == 0) {
        FunctionSpecificationMessage msg;
        switch (event) {
            case SERVER_API_ENTRY:
            {
                auto *arg_0 = msg.add_arg();
                nfc_event_t *arg_val_0 = reinterpret_cast<nfc_event_t*> ((*args)[0]);
                arg_0->set_type(TYPE_ENUM);
                profile__nfc_event_t(arg_0, (*arg_val_0));
                auto *arg_1 = msg.add_arg();
                nfc_status_t *arg_val_1 = reinterpret_cast<nfc_status_t*> ((*args)[1]);
                arg_1->set_type(TYPE_ENUM);
                profile__nfc_status_t(arg_1, (*arg_val_1));
                break;
            }
            case SERVER_API_EXIT:
            {
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        VtsProfilingInterface::getInstance().AddTraceEvent(msg);
    }
    if (strcmp(method, "sendData") == 0) {
        FunctionSpecificationMessage msg;
        switch (event) {
            case SERVER_API_ENTRY:
            {
                auto *arg_0 = msg.add_arg();
                nfc_data_t *arg_val_0 = reinterpret_cast<nfc_data_t*> ((*args)[0]);
                arg_0->set_type(TYPE_STRUCT);
                profile__nfc_data_t(arg_0, (*arg_val_0));
                break;
            }
            case SERVER_API_EXIT:
            {
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        VtsProfilingInterface::getInstance().AddTraceEvent(msg);
    }
}

}  // namespace vts
}  // namespace android
