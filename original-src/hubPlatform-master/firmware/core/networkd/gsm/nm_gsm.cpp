#include "networkd/gsm/nm_gsm.h"

#include <utility>
#include <unordered_map>

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {
namespace GSM {

namespace {
const std::unordered_map< std::string, GSMModem::AccessTechnology::Enum > accessTechnologyNames = {
    { "NONE", GSMModem::AccessTechnology::Enum::NONE },       { "GSM", GSMModem::AccessTechnology::Enum::GSM },
    { "GPRS", GSMModem::AccessTechnology::Enum::GPRS },       { "EDGE", GSMModem::AccessTechnology::Enum::EDGE },
    { "WCDMA", GSMModem::AccessTechnology::Enum::WCDMA },     { "HSDPA", GSMModem::AccessTechnology::Enum::HSPA_DL },
    { "HSUPA", GSMModem::AccessTechnology::Enum::HSPA_UP },   { "HSPA+", GSMModem::AccessTechnology::Enum::HSPA_PLUS },
    { "TDD LTE", GSMModem::AccessTechnology::Enum::LTE_TDD }, { "FDD LTE", GSMModem::AccessTechnology::Enum::LTE_FDD },
    { "OTHER", GSMModem::AccessTechnology::Enum::OTHER },
};
}

GSMModem::AccessTechnology::AccessTechnology()
: m_accessTechnology( Enum::NONE ) {}

GSMModem::AccessTechnology::AccessTechnology( const std::string& accessTechnology ) {
    auto technology = Enum::OTHER;

    auto accessTechnologyIt = accessTechnologyNames.find( accessTechnology );
    if( accessTechnologyIt != accessTechnologyNames.end() ) {
        technology = accessTechnologyIt->second;
    }

    m_accessTechnology = technology;
}

std::string GSMModem::AccessTechnology::toString() const {
    for( const auto& elm : accessTechnologyNames ) {
        if( elm.second == m_accessTechnology ) {
            return elm.first;
        }
    }

    return "UNKNOWN";
}

bool GSMModem::AccessTechnology::operator!=( const AccessTechnology& rhs ) const {
    return rhs.m_accessTechnology != m_accessTechnology;
}

bool GSMModem::AccessTechnology::isLTE() const {
    return m_accessTechnology == Enum::LTE_TDD || m_accessTechnology == Enum::LTE_FDD;
}

bool GSMModem::AccessTechnology::isHSPA() const {
    return m_accessTechnology == Enum::HSPA_UP || m_accessTechnology == Enum::HSPA_DL;
}

GSMModem::SIMInfo::SIMInfo()
: m_isSIMLocked( false ) {}

GSMModem::SIMInfo::SIMInfo( std::string iccid, std::string imsi, bool simLocked )
: m_iccid( std::move( iccid ) )
, m_imsi( std::move( imsi ) )
, m_isSIMLocked( simLocked ) {}

GSMModem::SIMInfo::operator bool() const {
    return !( m_iccid.empty() && m_imsi.empty() );
}

bool GSMModem::SIMInfo::operator!() const {
    return !this->operator bool();
}

GSMModem::GSMModem( bool embeddedModem )
: m_embeddedModem( embeddedModem ) {}

bool GSMModem::isEmbedded() const {
    return m_embeddedModem;
}

GSMModem::SignalQuality::SignalQuality()
: m_rssi( 0 )
, m_rsrp( 0 )
, m_rsrq( 0 )
, m_valid( false ) {}

GSMModem::SignalQuality::SignalQuality( const AccessTechnology& accessTechnology, int rssi )
: SignalQuality() {
    m_accessTechnology = accessTechnology;
    m_rssi = rssi;
    m_valid = true;
}

GSMModem::SignalQuality::SignalQuality( const AccessTechnology& accessTechnology, int rssi, int rsrp, int rsrq )
: SignalQuality() {
    m_accessTechnology = accessTechnology;
    m_rssi = rssi;
    m_rsrp = rsrp;
    m_rsrq = rsrq;
    m_valid = true;
}

GSMModem::SignalQuality::operator bool() const {
    return m_valid;
}

bool GSMModem::SignalQuality::operator!() const {
    return !this->operator bool();
}

GSMModem::MobileNetworkInfo::MobileNetworkInfo( std::string fnn, std::string spn, std::string rplmn )
: m_fnn( std::move( fnn ) )
, m_spn( std::move( spn ) )
, m_rplmn( std::move( rplmn ) ) {}

Json GSMModem::MobileNetworkInfo::toJson() const {
    return JSON( "fnn" << m_fnn << "spn" << m_spn << "rplmn" << m_rplmn );
}

GSMModem::MobileNetworkInfo::operator bool() const {
    return !( m_fnn.empty() || m_rplmn.empty() );
}

bool GSMModem::MobileNetworkInfo::operator!() const {
    return !this->operator bool();
}

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
