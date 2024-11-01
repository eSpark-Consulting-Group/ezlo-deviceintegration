#include "common_utils/hub_hardware_info.h"
#include "common_utils/formatters.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*

TESTPLAN

DONE    1. HubHardwareInfo tests
DONE        1.1 2Gig module
DONE        1.2 Battery gauge

*/

TEST( HubHardwareInfoTestSuite, TwoGigModule ) {  // 1.1
    using Info = HubHardwareInfo::HubHardwareInfo;

    EXPECT_EQ( Info( "", "" ).get2GIGInfo(), Info::Module2GIG::NotPresent );
    EXPECT_EQ( Info( "h2.1", "" ).get2GIGInfo(), Info::Module2GIG::NotPresent );

    EXPECT_EQ( Info( "h2_secure.1", "" ).get2GIGInfo(), Info::Module2GIG::Present );
    EXPECT_EQ( Info( "h2_secure.1", "E550_Rev_3.0" ).get2GIGInfo(), Info::Module2GIG::Present );

    EXPECT_EQ( Info( "h2_secure.1", "E550_Rev_4.0" ).get2GIGInfo(), Info::Module2GIG::NotPresent );
}

TEST( HubHardwareInfoTestSuite, BatteryGauge ) {  // 1.1
    using Info = HubHardwareInfo::HubHardwareInfo;

    EXPECT_EQ( Info( "", "" ).getBatteryGaugeInfo(), Info::ModuleBatteryGauge::NotPresent );
    EXPECT_EQ( Info( "h2.1", "" ).getBatteryGaugeInfo(), Info::ModuleBatteryGauge::NotPresent );

    EXPECT_EQ( Info( "h2_secure.1", "" ).getBatteryGaugeInfo(), Info::ModuleBatteryGauge::Hardware );

    EXPECT_EQ( Info( "h2_secure.1", "E550_Rev_3.0" ).getBatteryGaugeInfo(), Info::ModuleBatteryGauge::Software );
    EXPECT_EQ( Info( "h2_secure.1", "E550_Rev_4.0" ).getBatteryGaugeInfo(), Info::ModuleBatteryGauge::Software );
}

}  // namespace UT
}  // namespace HubPlatform
