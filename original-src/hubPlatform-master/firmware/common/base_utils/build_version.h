#pragma once

#include "base_utils/semantic_version.h"
#include "base_utils/verrevcmp.h"

#include <string>
#include <sstream>
#include <cstring>

namespace HubPlatform {
namespace Common {

class BuildVersion final {
public:
    BuildVersion( int major, int minor, std::string suffix = "" )
    : m_semanticVersion( major, minor )
    , m_suffix( std::move( suffix ) ) {}

    explicit BuildVersion( const std::string& ver ) {
        char buffer[ 256 ];
        memset( buffer, '\0', sizeof( buffer ) );
        int major = 0, minor = 0;
        std::sscanf( ver.c_str(), "%d.%d.%255s", &major, &minor, buffer );
        m_semanticVersion = HubPlatform::Common::SemanticVersion< int >( major, minor );
        m_suffix = buffer;
    }

    BuildVersion() {}

    bool isValid() const {
        return m_semanticVersion.isValid();
    }

    bool operator==( const BuildVersion& rhs ) const {
        return m_semanticVersion == rhs.m_semanticVersion &&
               Common::verrevcmp( m_suffix.c_str(), rhs.m_suffix.c_str() ) == 0;
    }

    bool operator<( const BuildVersion& other ) const {
        auto lhs = std::make_tuple( getMajor(), getMinor() );
        auto rhs = std::make_tuple( other.getMajor(), other.getMinor() );
        if( lhs < rhs )
            return true;
        if( lhs > rhs )
            return false;

        return Common::verrevcmp( m_suffix.c_str(), other.m_suffix.c_str() ) < 0;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << m_semanticVersion.toString();
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

    const std::string& getSuffix() const {
        return m_suffix;
    }

private:
    HubPlatform::Common::SemanticVersion< int > m_semanticVersion;
    std::string m_suffix;
};

}  // namespace Common
}  // namespace HubPlatform

namespace std {
template <>
struct hash< HubPlatform::Common::BuildVersion > {
    size_t operator()( const HubPlatform::Common::BuildVersion& version ) const {
        size_t result{};
        common::hash_combine( result, common::hash_value( version.getMajor() ) );
        common::hash_combine( result, common::hash_value( version.getMinor() ) );
        common::hash_combine( result, common::hash_value( version.getSuffix() ) );
        return result;
    }
};
}  // namespace std
