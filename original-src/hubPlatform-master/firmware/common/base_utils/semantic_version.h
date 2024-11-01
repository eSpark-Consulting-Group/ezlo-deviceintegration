#pragma once

#include "base_utils/hash_utils.h"

#include <string>
#include <sstream>
#include <tuple>

namespace HubPlatform {
namespace Common {

template < typename T >
class SemanticVersion final {
public:
    using ComponentType = T;

    constexpr SemanticVersion( T major, T minor )
    : m_major( major )
    , m_minor( minor ) {}

    explicit SemanticVersion( const std::string& ver ) {
        std::sscanf( ver.c_str(), "%d.%d", &m_major, &m_minor );
    }

    SemanticVersion() {}

    template < typename U >
    bool operator<( const SemanticVersion< U >& rhs ) const {
        return std::tie( m_major, m_minor ) < std::tie( rhs.m_major, rhs.m_minor );
    }

    template < typename U >
    bool operator>( const SemanticVersion< U >& rhs ) const {
        return std::tie( m_major, m_minor ) > std::tie( rhs.m_major, rhs.m_minor );
    }

    bool isValid() const {
        return m_major > 0 || m_minor > 0;
    }

    bool operator==( const SemanticVersion& rhs ) const {
        return m_major == rhs.m_major && m_minor == rhs.m_minor;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << +m_major << '.' << +m_minor;
        return ss.str();
    }

    int getMajor() const {
        return m_major;
    }

    int getMinor() const {
        return m_minor;
    }

private:
    T m_major = T{};
    T m_minor = T{};
};

}  // namespace Common
}  // namespace HubPlatform

namespace std {
template < typename T >
struct hash< HubPlatform::Common::SemanticVersion< T > > {
    size_t operator()( const HubPlatform::Common::SemanticVersion< T >& version ) const {
        return common::hash_combine( common::hash_value( version.getMajor() ),
                                     common::hash_value( version.getMinor() ) );
    }
};
}  // namespace std
