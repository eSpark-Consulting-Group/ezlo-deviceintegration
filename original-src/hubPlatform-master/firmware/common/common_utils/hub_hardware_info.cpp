#include "common_utils/hub_hardware_info.h"

namespace HubHardwareInfo {
namespace Revisions {
// NOTE: Hardware revision is stored within provisioning data
const char* const E550_REV_3_0 = "E550_Rev_3.0";  // Revision without hardware battery gauge. LTE module is used instead
const char* const E550_REV_4_0 = "E550_Rev_4.0";  // Revision without 2GIG module
}  // namespace Revisions

HubHardwareInfo::HubHardwareInfo( std::string hubType, std::string hardwareRevision )
: m_hubType( std::move( hubType ) )
, m_hardwareRevision( std::move( hardwareRevision ) ) {}

HubHardwareInfo::ModuleBatteryGauge HubHardwareInfo::getBatteryGaugeInfo() {
    using namespace Revisions;
    if( m_hubType != "h2_secure.1" ) {
        return ModuleBatteryGauge::NotPresent;
    }

    if( m_hardwareRevision == E550_REV_3_0 || m_hardwareRevision == E550_REV_4_0 ) {
        return ModuleBatteryGauge::Software;
    }

    return ModuleBatteryGauge::Hardware;
}

HubHardwareInfo::Module2GIG HubHardwareInfo::get2GIGInfo() {
    using namespace Revisions;
    if( m_hubType != "h2_secure.1" ) {
        return Module2GIG::NotPresent;
    }

    if( m_hardwareRevision == E550_REV_4_0 ) {
        return Module2GIG::NotPresent;
    }

    return Module2GIG::Present;
}

}  // namespace HubHardwareInfo
