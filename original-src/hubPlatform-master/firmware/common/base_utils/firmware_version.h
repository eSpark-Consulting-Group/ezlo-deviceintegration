#pragma once

#include "base_utils/semantic_version.h"
#include "base_utils/verrevcmp.h"

#include <cstdint>
#include <cstring>

namespace HubPlatform {
namespace Firmware {

class Version final {
public:
    Version( int major, int minor, int sub, std::string suffix = "" )
    : m_semanticVersion( major, minor )
    , m_sub( sub )
    , m_suffix( std::move( suffix ) ) {}

    explicit Version( const std::string& ver ) {
        char buffer[ 256 ];
        memset( buffer, '\0', sizeof( buffer ) );
        int major = 0, minor = 0;
        std::sscanf( ver.c_str(), "%d.%d.%d.%255s", &major, &minor, &m_sub, buffer );
        m_semanticVersion = Common::SemanticVersion< int >( major, minor );
        m_suffix = buffer;
    }

    Version() {}

    bool isValid() const {
        return m_semanticVersion.isValid();
    }

    bool operator==( const Version& rhs ) const {
        return m_semanticVersion == rhs.m_semanticVersion && m_sub == rhs.m_sub &&
               Common::verrevcmp( m_suffix.c_str(), rhs.m_suffix.c_str() ) == 0;
    }

    bool operator<( const Version& other ) const {
        auto lhs = std::make_tuple( getMajor(), getMinor(), getSub() );
        auto rhs = std::make_tuple( other.getMajor(), other.getMinor(), other.getSub() );
        if( lhs < rhs )
            return true;
        if( lhs > rhs )
            return false;

        return Common::verrevcmp( m_suffix.c_str(), other.m_suffix.c_str() ) < 0;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << m_semanticVersion.toString();
        if( m_sub )
            ss << '.' << +m_sub;
        if( !m_suffix.empty() )
            ss << '.' << m_suffix;
        return ss.str();
    }

    int getMajor() const {
        return m_semanticVersion.getMajor();
    }

    int getMinor() const {
        return m_semanticVersion.getMinor();
    }

    int getSub() const {
        return m_sub;
    }

    const std::string& getSuffix() const {
        return m_suffix;
    }

private:
    Common::SemanticVersion< int > m_semanticVersion;
    int m_sub = {};
    std::string m_suffix;
};

}  // namespace Firmware
}  // namespace HubPlatform

namespace std {
template <>
struct hash< HubPlatform::Firmware::Version > {
    size_t operator()( const HubPlatform::Firmware::Version& version ) const {
        size_t result{};
        common::hash_combine( result, common::hash_value( version.getMajor() ) );
        common::hash_combine( result, common::hash_value( version.getMinor() ) );
        common::hash_combine( result, common::hash_value( version.getSub() ) );
        common::hash_combine( result, common::hash_value( version.getSuffix() ) );
        return result;
    }
};
}  // namespace std
