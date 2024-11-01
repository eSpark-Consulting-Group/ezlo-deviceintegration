#include "nm_scan_result.h"
#include "networkd/resources/nm_resources.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {

bool NetworkItem::isValid() const {
    return !m_ssid.empty() && !m_bssid.empty() && m_channel != -1;
}

Json NetworkItem::toJson( bool auxFields ) const {
    Json item;
    using namespace Resources;
    item[ std::string( Scan::ChannelKey ) ] = m_channel;
    item[ std::string( Scan::SsidKey ) ] = m_ssid;
    item[ std::string( Scan::BssidKey ) ] = m_bssid;
    item[ std::string( Scan::SignalLevelValueKey ) ] = m_signalPercentLevel;
    item[ std::string( Scan::WifiSecurityKey ) ] = m_security;
    if( auxFields ) {
        item[ Resources::Scan::Aux ] = JSON(
            Scan::AuxFields::Band << m_aux.m_band << Scan::AuxFields::Frequency << m_aux.m_frequency
                                  << Scan::AuxFields::Associated << m_aux.m_associated << Scan::AuxFields::Signal
                                  << m_aux.m_signal << Scan::AuxFields::Capabilities << m_aux.m_capabilitiesFlags );
    }
    return item;
}

bool NetworkItem::fromJson( const Json& item, bool auxFields ) {
    NetworkItem& networkItem = *this;
    networkItem.m_channel = json_get_int( item, Resources::Scan::ChannelKey );
    networkItem.m_ssid = json_get_str( item, Resources::Scan::SsidKey );
    networkItem.m_bssid = json_get_str( item, Resources::Scan::BssidKey );
    networkItem.m_signalPercentLevel = json_get_int( item, Resources::Scan::SignalLevelValueKey );
    networkItem.m_security = json_get_str( item, Resources::Scan::WifiSecurityKey );
    if( auxFields ) {
        Json aux;
        using namespace Resources;
        if( json_get_obj( item, Resources::Scan::Aux, aux ) ) {
            networkItem.m_aux.m_band = json_get_str( aux, Scan::AuxFields::Band );
            networkItem.m_aux.m_associated = json_get_bool( aux, Scan::AuxFields::Associated );
            networkItem.m_aux.m_signal = json_get_float( aux, Scan::AuxFields::Signal );
            networkItem.m_aux.m_frequency = json_get_int( aux, Scan::AuxFields::Frequency );
            networkItem.m_aux.m_capabilitiesFlags = json_get_int( aux, Scan::AuxFields::Capabilities );
        }
    }
    return true;
}

void NetworkItem::clear() {
    *this = NetworkItem();
}

}  // namespace Network
}  // namespace HubPlatform