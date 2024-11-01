#pragma once

#include <dbus/dbus.h>

#include <string>

namespace HubPlatform {

template < typename CppType >
struct cpp_type_to_dbus_type {};

template <>
struct cpp_type_to_dbus_type< bool > {
    using value = dbus_bool_t;
};

template <>
struct cpp_type_to_dbus_type< std::string > {
    using value = const char*;
};

template <>
struct cpp_type_to_dbus_type< std::uint32_t > {
    using value = dbus_uint32_t;
};

template <>
struct cpp_type_to_dbus_type< std::uint16_t > {
    using value = dbus_uint16_t;
};

template <>
struct cpp_type_to_dbus_type< std::int16_t > {
    using value = dbus_int16_t;
};

template <>
struct cpp_type_to_dbus_type< std::uint8_t > {
    using value = unsigned char;
};

template < typename CppType >
using cpp_type_to_dbus_type_t = typename cpp_type_to_dbus_type< CppType >::value;
}  // namespace HubPlatform
