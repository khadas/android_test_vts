# Codelab - VTS Specification File Development

VTS uses a specification file which is either manually written by a developer
for a non-HIDL HAL, or automatically generated from a .hal file for a HIDL HAL.
The VTS Compiler parses a VTS specification file (.vts file) and produces
both target-side native and host-side python code that is needed for Android
system testing.

A VTS specification flie is an ASCII protobuf file whose format is defined in
[InterfaceSpecificationMessage.proto](https://googleplex-android.git.corp.google.com/platform/test/vts/+/master/proto/InterfaceSpecificationMessage.proto).
Please check [this web site](https://developers.google.com/protocol-buffers/)
if you are not yet familiar with Protocol Buffer.

## 1. For HIDL HAL

To automatically generate a .vts file from .hal file, please check
[this manual](https://googleplex-android.git.corp.google.com/platform/system/tools/hidl/+/master/README.md).

## 2. For non-HIDL HAL (e.g., Conventional HAL, Shared Library, and Kernel)

### 2.1. Global Properties

First let us set some global flags that are always required. The following
are the required fields and their possible values. The full list is available
in the InterfaceSpecificationMessage.proto file.

- component_class
 * `HAL_CONVENTIONAL` (for a conventional HAL)
 * `HAL_CONVENTIONAL_SUBMODULE` (for a submodule, e.g., nested APIs, of a conventional HAL)
 * `HAL_LEGACY` (for a legacy HAL, e.g., shared lib style)
 * `HAL_HIDL` (for a HIDL HAL)
 * `HAL_HIDL_WRAPPED_CONVENTIONAL` (for a HIDL wrapped conventional HAL)
 * `LIB_SHARED` (for a shared library)

- component_type
 * `AUDIO` (for an audio HAL)
 * `CAMERA` (for a camera HAL)
 * `GPS` (for a GPS HAL)
 * `LIGHT` (for a lights HAL)
 * `WIFI` (for a WiFi HAL)
 * `MOBILE` (for a mobile data HAL)
 * `BLUETOOTH` (for a Bluetooth HAL)
 * `NFC` (for a near-field communication HAL)
 * `BIONIC_LIBM` (for a libm which is part of bionic library)
 * `VNDK_LIBCUTILS` (for a libcutils which is part of Vendor NDK)

- component_type_version
 * a floating-point number (e.g., 1.0)

There are some optional global fields.

- original_data_structure_name
  * the corresponding C data structure (e.g., "`struct gps_device_t`" for a conventional GPS HAL).

- header
  * this's a repeated field and used to list all the required headers
   (e.g., `"<hardware/hardware.h>"` and `"<hardware/gps.h>"` for a conventional GPS HAL).

Based on those rules, the first part of a final vts file for a conventional
lights HAL looks like this:

---
```
component_class: HAL_CONVENTIONAL
component_type: LIGHT
component_type_version: 1.0

original_data_structure_name: "struct light_device_t"

header: "<hardware/hardware.h>"
header: "<hardware/lights.h>"
```
---

### 2.2. APIs

Then each API which you would like to call from the host-side Python
 needs to be defined as an `api` field. An `api` field is a
 `FunctionSpecificationMessage` which is defined in the
 `InterfaceSpecificationMesage.proto` file.

It has multiple sub fields (where only some are needed for each type).

 - `name`: function name
 - `submodule_name`: in case it is a sub module, its module name

The return value type is specified using one of:

 - `return_type` (`VariableSpecificationMessage`): data type of the return
 value (for a legacy HAL, conventional HAL, and shared library)
 - `return_type_hidl` (`VariableSpecificationMessage` repeated field): data
 type of the return value (for a HIDL HAL)

The argument type is specified in:

 - arg (`VariableSpecificationMessage` repeated field): a list of arguments.

Some other property fields are:

 - `is_callback`: true if it is a callback API.

A `VariableSpecificationMessage` is the mostly widely used because that is used
to describe an argument. Its key fields are:

 - `name`: the variable name. empty if for a type definition.

 - `type`: the variable type which is one of: `TYPE_SCALAR`, `TYPE_STRING`,
 `TYPE_ENUM`, `TYPE_ARRAY`, `TYPE_VECTOR`, `TYPE_STRUCT`, and `TYPE_UNION`.

 - `scalar_type`: if its type is `TYPE_SCALAR` and possible values are:
  "`bool_t`", "`int8_t`", "`uint8_t`", "`char`", "`uchar`", "`int16_t`",
  "`uint16_t`", "`int32_t`", "`uint32_t`", "`int64_t`", "`uint64_t`",
  "`float_t`", "`double_t`", and so on.

 - `string_value` (`StringDataValueMessage`): if its type is `TYPE_STRING`.

 - `enum_value` (`EnumDataValueMessage`): if its type is `TYPE_ENUM`.

 - `vector_value` (`VectorDataValueMessage`): if its type is either `TYPE_VECTOR`
  or `TYPE_ARRAY`

 - `struct_type` and `struct_value`: if its type is `TYPE_STRUCT`

 - `union_value`: if its type is `TYPE_UNION`.

It also has some property fields:

 - `is_input`: if an argument is an input (by default true).
 - `is_output`: if an argument is an output, e.g., pointer (by default false).
 - `is_const`: if an argument is a constant (by default false).
 - `is_callback`: if an argument is a callback (by default false).

### 2.3. Attributes

Attributes can also be defined as part of a VTS specification file
so that one can easily create memory instances of them on the Python-side and
transfer them to a target device. Without an attribute definition, it is not
always possibile for the VTS framework to know how to recreate an instance of
an attribute on the target side (especially when that attribute is not using
a type which is pre-defined in an existing, imported C/C++ header file).

In order to define an attribute, one can use `attribute` field which is a
repeated `VariableSpecificationMessage`.

### 2.4. Examples

The following are some example `VariableSpecificationMessage` specifications,
which can be used for an argument or an attribute:

For a 32-bit signed integer argument,
```
arg: {
  type: TYPE_SCALAR
  scalar_type: "int32_t"
}
```

For a char* argument,
```
arg: {
  type: TYPE_SCALAR
  scalar_type: "char_pointer"
}
```
for a non-HIDL HAL, and
```
arg: {
  type: TYPE_STRING
}
```
for a HIDL HAL.

For a struct argument which contains a unsigned char vector,
```
arg: {
  type: TYPE_STRUCT
  predefined_type: "nfc_data_t"
}

attribute: {
  name: "nfc_data_t"
  type: TYPE_STRUCT
  struct_value: {
    name: "data"
    type: TYPE_VECTOR
    vector_value: {
      scalar_type: "uint8_t"
    }
  }
}
```

To define an enum type,
```
attribute: {
  name: "nfc_event_t"
  type: TYPE_ENUM
  enum_value: {
    enumerator: "HAL_NFC_OPEN_CPLT_EVT"
    value: 0
    enumerator: "HAL_NFC_CLOSE_CPLT_EVT"
    value: 1
    enumerator: "HAL_NFC_POST_INIT_CPLT_EVT"
    value: 2
    enumerator: "HAL_NFC_PRE_DISCOVER_CPLT_EVT"
    value: 3
    enumerator: "HAL_NFC_REQUEST_CONTROL_EVT"
    value: 4
    enumerator: "HAL_NFC_RELEASE_CONTROL_EVT"
    value: 5
    enumerator: "HAL_NFC_ERROR_EVT"
    value: 6
  }
}
```
