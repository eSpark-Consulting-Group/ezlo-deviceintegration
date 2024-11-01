#pragma once

#include <map>
#include <unordered_map>

namespace HubPlatform {
namespace TwoGig {
namespace Resources {

template < typename EnumT >
struct EnumUtils {
    using Enum = typename EnumT::Enum;

    static Enum fromString( const std::string& str );
    static const std::string& toString( Enum value );

private:
    EnumUtils( std::map< std::string, Enum >&& lookup );

private:
    static EnumUtils< EnumT > ms_impl;

    std::map< std::string, Enum > m_valueByString;
    std::unordered_map< typename std::underlying_type< Enum >::type, const std::string* > m_stringByValue;
};

}  // namespace Resources
}  // namespace TwoGig
}  // namespace HubPlatform
