#pragma once
#include <string>

namespace HubPlatform {

namespace UT {
class MdnsConfigSuite;
}

class MdnsConfig {
public:
    static bool runMdns( int localServerPort );

private:
    static bool replaceString( std::string& str, const std::string& toReplace, const std::string& replaceWith );
    static bool setAvahiHost( const std::string& fileName );
    static bool setAvahiService( int localServerPort, const std::string& avahiServicesPath );

    friend class UT::MdnsConfigSuite;
};

}  // namespace HubPlatform
