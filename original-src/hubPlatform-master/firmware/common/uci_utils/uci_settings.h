#pragma once

#include <map>
#include <vector>

#include <ostream>
#include <istream>

namespace HubPlatform {
namespace UciUtils {

struct UciSection {
    bool isValid() const {
        return !m_type.empty() || !m_name.empty();
    }

    std::string m_type;
    std::string m_name;

    using SingleOptions = std::map< std::string, std::string >;
    using ListedOptions = std::multimap< std::string, std::string >;

    SingleOptions m_singleOptions;
    ListedOptions m_listedOptions;
};

bool operator==( const UciSection& first, const UciSection& second );
bool operator!=( const UciSection& first, const UciSection& second );

using UciSettings = std::vector< UciSection >;

std::ostream& operator<<( std::ostream& stream, const UciSection& section );
std::ostream& operator<<( std::ostream& stream, const UciSettings& settings );
std::istream& operator>>( std::istream& stream, UciSettings& settings );

bool readSettings( const std::string& fileName, UciUtils::UciSettings& settings );
bool writeSettings( const std::string& fileName, const UciUtils::UciSettings& settings );

}  // namespace UciUtils
}  // namespace HubPlatform
