#include "networkd/configuration/nm_config_manager.h"
#include "networkd/resources/nm_resources.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {

Json ModeConfiguration::toJson() const {
    using namespace Resources::InterfaceInfo;
    Json result;

    result[ Fields::Mode ] = m_mode;

    if( m_mode == "static" ) {
        result[ Fields::Ip ] = m_ip;
        result[ Fields::Mask ] = m_netMask;
    }

    return result;
}

bool EthernetConfiguration::isEmtpy() const {
    return m_mode.m_mode.empty();
}

Json EthernetConfiguration::toJson() const {
    return JSON( "mode" << m_mode.toJson() );
}

bool APWifiConfiguration::isEmtpy() const {
    return m_ssid.empty() || m_encryption.empty() || m_channel == 0;
}

Json APWifiConfiguration::toJson() const {
    return JSON( "ssid" << m_ssid << "encryption" << m_encryption << "channel" << m_channel << "ipLimit" << m_ipLimit
                        << "mode" << m_mode.toJson() );
}

bool ClientWifiConfiguration::isEmtpy() const {
    return m_ssid.empty() || m_encryption.empty() || m_channel == 0;
}

Json ClientWifiConfiguration::toJson() const {
    return JSON( "ssid" << m_ssid << "encryption" << m_encryption << "channel" << m_channel << "bssid" << m_bssid
                        << "mode" << m_mode.toJson() );
}

bool MobileConfiguration::isEmtpy() const {
    return m_device.empty() || m_pppName.empty();
}

Json MobileConfiguration::toJson() const {
    return JSON( "apn" << m_apn << "device" << m_device << "pppName" << m_pppName << "dialNumber" << m_dialNumber
                       << "isIPv6" << m_ipv6 << "mode" << m_mode );
}

}  // namespace Network
}  // namespace HubPlatform
