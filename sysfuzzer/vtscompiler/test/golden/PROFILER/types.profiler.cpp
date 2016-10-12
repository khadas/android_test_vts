#include "hardware/interfaces/nfc/1.0/vts/types.vts.h"
#include "hardware/interfaces/nfc/1.0/vts/types.vts.h"

using namespace android::hardware;
using namespace android::hardware::nfc::V1_0;

namespace android {
namespace vts {

void profile__nfc_event_t(VariableSpecificationMessage* arg_name,
nfc_event_t arg_val_name) {
    arg_name->set_type(TYPE_ENUM);
    arg_name->mutable_enum_value()->add_scalar_value()->set_uint32_t(static_cast<uint32_t>(arg_val_name));
    arg_name->mutable_enum_value()->set_scalar_type("uint32_t");
}

void profile__nfc_status_t(VariableSpecificationMessage* arg_name,
nfc_status_t arg_val_name) {
    arg_name->set_type(TYPE_ENUM);
    arg_name->mutable_enum_value()->add_scalar_value()->set_uint32_t(static_cast<uint32_t>(arg_val_name));
    arg_name->mutable_enum_value()->set_scalar_type("uint32_t");
}

void profile__nfc_data_t(VariableSpecificationMessage* arg_name,
nfc_data_t arg_val_name) {
    arg_name->set_type(TYPE_STRUCT);
    auto *arg_name_data = arg_name->add_struct_value();
    for (int i = 0; i < (int)arg_val_name.data.size(); i++) {
        auto *arg_name_data_vector_i = arg_name_data->add_vector_value();
        arg_name_data_vector_i->set_type(TYPE_SCALAR);
        arg_name_data_vector_i->mutable_scalar_value()->set_uint8_t(arg_val_name.data[i]);
    }
}

}  // namespace vts
}  // namespace android
