#include "hardware/interfaces/nfc/1.0/vts/Nfc.vts.h"
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
    if (strcmp(interface, "INfc") != 0) {
        LOG(WARNING) << "incorrect interface.";
        return;
    }

    char trace_file[PATH_MAX];
    sprintf(trace_file, "%s/%s@%s", TRACEFILEPREFIX, package, version);
    VtsProfilingInterface& profiler = VtsProfilingInterface::getInstance(trace_file);
    profiler.Init();

    if (strcmp(method, "open") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("open");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            case HidlInstrumentor::PASSTHROUGH_ENTRY:
            {
                auto *arg_0 = msg.add_arg();
                INfcClientCallback *arg_val_0 = reinterpret_cast<INfcClientCallback*> ((*args)[0]);
                arg_0->set_type(TYPE_HIDL_CALLBACK);
                break;
            }
            case HidlInstrumentor::CLIENT_API_EXIT:
            case HidlInstrumentor::SERVER_API_EXIT:
            case HidlInstrumentor::PASSTHROUGH_EXIT:
            {
                auto *result_0 = msg.add_return_type_hidl();
                int32_t *result_val_0 = reinterpret_cast<int32_t*> ((*args)[0]);
                result_0->set_type(TYPE_SCALAR);
                result_0->mutable_scalar_value()->set_int32_t((*result_val_0));
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "write") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("write");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            case HidlInstrumentor::PASSTHROUGH_ENTRY:
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
            case HidlInstrumentor::PASSTHROUGH_EXIT:
            {
                auto *result_0 = msg.add_return_type_hidl();
                int32_t *result_val_0 = reinterpret_cast<int32_t*> ((*args)[0]);
                result_0->set_type(TYPE_SCALAR);
                result_0->mutable_scalar_value()->set_int32_t((*result_val_0));
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "coreInitialized") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("coreInitialized");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            case HidlInstrumentor::PASSTHROUGH_ENTRY:
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
            case HidlInstrumentor::PASSTHROUGH_EXIT:
            {
                auto *result_0 = msg.add_return_type_hidl();
                int32_t *result_val_0 = reinterpret_cast<int32_t*> ((*args)[0]);
                result_0->set_type(TYPE_SCALAR);
                result_0->mutable_scalar_value()->set_int32_t((*result_val_0));
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "prediscover") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("prediscover");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            case HidlInstrumentor::PASSTHROUGH_ENTRY:
            {
                break;
            }
            case HidlInstrumentor::CLIENT_API_EXIT:
            case HidlInstrumentor::SERVER_API_EXIT:
            case HidlInstrumentor::PASSTHROUGH_EXIT:
            {
                auto *result_0 = msg.add_return_type_hidl();
                int32_t *result_val_0 = reinterpret_cast<int32_t*> ((*args)[0]);
                result_0->set_type(TYPE_SCALAR);
                result_0->mutable_scalar_value()->set_int32_t((*result_val_0));
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "close") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("close");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            case HidlInstrumentor::PASSTHROUGH_ENTRY:
            {
                break;
            }
            case HidlInstrumentor::CLIENT_API_EXIT:
            case HidlInstrumentor::SERVER_API_EXIT:
            case HidlInstrumentor::PASSTHROUGH_EXIT:
            {
                auto *result_0 = msg.add_return_type_hidl();
                int32_t *result_val_0 = reinterpret_cast<int32_t*> ((*args)[0]);
                result_0->set_type(TYPE_SCALAR);
                result_0->mutable_scalar_value()->set_int32_t((*result_val_0));
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "controlGranted") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("controlGranted");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            case HidlInstrumentor::PASSTHROUGH_ENTRY:
            {
                break;
            }
            case HidlInstrumentor::CLIENT_API_EXIT:
            case HidlInstrumentor::SERVER_API_EXIT:
            case HidlInstrumentor::PASSTHROUGH_EXIT:
            {
                auto *result_0 = msg.add_return_type_hidl();
                int32_t *result_val_0 = reinterpret_cast<int32_t*> ((*args)[0]);
                result_0->set_type(TYPE_SCALAR);
                result_0->mutable_scalar_value()->set_int32_t((*result_val_0));
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "powerCycle") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("powerCycle");
        switch (event) {
            case HidlInstrumentor::CLIENT_API_ENTRY:
            case HidlInstrumentor::SERVER_API_ENTRY:
            case HidlInstrumentor::PASSTHROUGH_ENTRY:
            {
                break;
            }
            case HidlInstrumentor::CLIENT_API_EXIT:
            case HidlInstrumentor::SERVER_API_EXIT:
            case HidlInstrumentor::PASSTHROUGH_EXIT:
            {
                auto *result_0 = msg.add_return_type_hidl();
                int32_t *result_val_0 = reinterpret_cast<int32_t*> ((*args)[0]);
                result_0->set_type(TYPE_SCALAR);
                result_0->mutable_scalar_value()->set_int32_t((*result_val_0));
                break;
            }
            default:
            {
                LOG(WARNING) << "not supported. ";
                break;
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
}

}  // namespace vts
}  // namespace android
