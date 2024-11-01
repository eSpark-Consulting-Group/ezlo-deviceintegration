#include "networkd/interfaces/nm_ap_interface_settings.h"

#include <json_utils/json_utils.h>

namespace HubPlatform {
namespace Network {

namespace {
constexpr char APName[] = "apName";
}

bool APInterfaceSettings::APSettings::empty() const {
    return m_isEmpty;
}

APInterfaceSettings::APInterfaceSettings( IDbClient& dbClient,
                                          std::string interfaceName,
                                          EnablePolicy defaultEnablePolicy )
: InterfaceSettings( dbClient, std::move( interfaceName ), defaultEnablePolicy ) {}

APInterfaceSettings::APSettings APInterfaceSettings::getAPSettings( Json customSettings ) {
    APSettings apSettings;

    if( !customSettings.empty() ) {
        apSettings.m_isEmpty = false;
        apSettings.m_apName = json_get_str( customSettings, APName );
    }

    return apSettings;
}

APInterfaceSettings::APSettings APInterfaceSettings::getAPSettings() const {
    return getAPSettings( getCustomSettings() );
}

bool APInterfaceSettings::setAPSettings( APSettings apSettings ) {
    Json customSettings;

    if( !checkAPSettings( apSettings ) ) {
        return false;
    }

    if( !apSettings.empty() ) {
        customSettings[ APName ] = apSettings.m_apName;
    }

    setCustomSettings( std::move( customSettings ) );

    return true;
}

bool APInterfaceSettings::checkAPSettings( const APSettings& apSettings ) {
    if( apSettings.empty() ) {
        return true;
    }

    return apSettings.m_apName.size() <= 32;
}

}  // namespace Network
}  // namespace HubPlatform
