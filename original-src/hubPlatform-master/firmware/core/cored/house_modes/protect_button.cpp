#include "cored/house_modes/protect_button.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreDHouseModes {

ProtectButton::ProtectButton( std::string deviceId, ProtectServiceEnum service )
: m_deviceId( std::move( deviceId ) )
, m_service( service ) {}

bool ProtectButton::empty() const {
    return m_deviceId.empty();
}

const std::string& ProtectButton::deviceId() const {
    return m_deviceId;
}

void ProtectButton::deviceId( const std::string& deviceId ) {
    m_deviceId = deviceId;
}

ProtectServiceEnum ProtectButton::service() const {
    return m_service;
}

void ProtectButton::service( ProtectServiceEnum service ) {
    m_service = service;
}

Json ProtectButton::toJson() const {
    return { { "deviceId", m_deviceId }, { "service", toString( m_service ) } };
}

bool ProtectButton::fromJson( const Json& source ) {
    auto id = json_get_str( source, "deviceId" );
    const auto protectService = json_get_str( source, "service" );
    auto serviceEnum = toProtectService( protectService );

    m_deviceId = std::move( id );
    m_service = serviceEnum;

    return true;
}

bool ProtectButton::fromDbJson( const Json& source ) {
    auto id = json_get_str( source, "_id" );
    const auto protectService = json_get_str( source, "service" );
    auto serviceEnum = toProtectService( protectService );

    m_deviceId = std::move( id );
    m_service = serviceEnum;

    return true;
}

bool ProtectButton::operator==( const ProtectButton& r ) const {
    return m_service == r.m_service && m_deviceId == r.m_deviceId;
}

}  // namespace CoreDHouseModes
}  // namespace HubPlatform
