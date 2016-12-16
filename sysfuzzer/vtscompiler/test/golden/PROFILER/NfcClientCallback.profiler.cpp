#include "hardware/interfaces/nfc/1.0/vts/NfcClientCallback.vts.h"
#include "hardware/interfaces/nfc/1.0/vts/types.vts.h"

using namespace android::hardware::nfc::V1_0;
using namespace android::hardware;

#define TRACEFILEPREFIX "/data/local/tmp"

namespace android {
namespace vts {


void HIDL_INSTRUMENTATION_FUNCTION(
        HidlInstrumentor::InstrumentationEvent event,
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

    char trace_file[PATH_MAX];
    sprintf(trace_file, "%s/%s@%s", TRACEFILEPREFIX, package, version);
    VtsProfilingInterface& profiler = VtsProfilingInterface::getInstance(trace_file);
    profiler.Init();

    if (strcmp(method, "sendEvent") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("sendEvent");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            {
                auto *arg_0 = msg.add_arg();
                ::android::hardware::nfc::V1_0::NfcEvent *arg_val_0 = reinterpret_cast<::android::hardware::nfc::V1_0::NfcEvent*> ((*args)[0]);
                arg_0->set_type(TYPE_ENUM);
                profile____android__hardware__nfc__V1_0__NfcEvent(arg_0, (*arg_val_0));
                auto *arg_1 = msg.add_arg();
                ::android::hardware::nfc::V1_0::NfcStatus *arg_val_1 = reinterpret_cast<::android::hardware::nfc::V1_0::NfcStatus*> ((*args)[1]);
                arg_1->set_type(TYPE_ENUM);
                profile____android__hardware__nfc__V1_0__NfcStatus(arg_1, (*arg_val_1));
                break;
            }
            case HidlInstrumentor::CLIENT_API_EXIT:
            case HidlInstrumentor::SERVER_API_EXIT:
            {
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(package, version, interface, msg);
    }
    if (strcmp(method, "sendData") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("sendData");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            {
                auto *arg_0 = msg.add_arg();
                ::android::hardware::hidl_vec<uint8_t> *arg_val_0 = reinterpret_cast<::android::hardware::hidl_vec<uint8_t>*> ((*args)[0]);
                for (int i = 0; i < (int)(*arg_val_0).size(); i++) {
                    auto *arg_0_vector_i = arg_0->add_vector_value();
                    arg_0_vector_i->set_type(TYPE_SCALAR);
                    arg_0_vector_i->mutable_scalar_value()->set_uint8_t((*arg_val_0)[i]);
                }
                break;
            }
            case HidlInstrumentor::CLIENT_API_EXIT:
            case HidlInstrumentor::SERVER_API_EXIT:
            {
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(package, version, interface, msg);
    }
}

}  // namespace vts
}  // namespace android
