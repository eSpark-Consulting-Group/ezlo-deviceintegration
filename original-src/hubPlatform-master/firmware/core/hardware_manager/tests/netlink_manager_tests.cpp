/*

TESTPLAN

DONE    1. Uevent tests
DONE        1.1 Empty uevent source
DONE        1.2 Uevent source has no summary field
DONE        1.3 Uevent source has no mandatory field
DONE        1.4 Uevent source has incorrect mandatory field
DONE        1.5 Uevent source has incorrect field
DONE        1.6 Uevent source is valid, but has unsupported action
DONE        1.7 Uevent source is valid, but has unsupported subsystem
DONE        1.8 Uevent source is valid

*/

#include "hardware_manager/netlink_manager/netlink_manager.h"

#include <gtest/gtest.h>

#define MAKE_STRING_WITH_NULL_SYMBOLS( name, data ) std::string name( data, sizeof( data ) );

namespace HubPlatform {
namespace HardwareManager {
namespace UT {

class NetlinkManagerTestSuite : public ::testing::Test {
protected:
    using UEvent = NetlinkManager::UEvent;
};

TEST_F( NetlinkManagerTestSuite, ueventEmptySource ) /*1.1*/ {
    ASSERT_THROW( UEvent( "" ), NetlinkManagerException );
}

TEST_F( NetlinkManagerTestSuite, ueventSourceHasNoSummaryField ) /*1.2*/ {
    ASSERT_THROW( UEvent( "1" ), NetlinkManagerException );
}

TEST_F( NetlinkManagerTestSuite, ueventSourceHasNoMandatoryField ) /*1.3*/ {
    MAKE_STRING_WITH_NULL_SYMBOLS( uevent, "add@/devices/platform/soc/1c1b000.usb/usb3/3-1" );
    ASSERT_THROW( UEvent{ uevent }, NetlinkManagerException );
}

TEST_F( NetlinkManagerTestSuite, ueventSourceIncorrectMandatoryField ) /*1.4*/ {
    MAKE_STRING_WITH_NULL_SYMBOLS( uevent,
                                   "add@/devices/platform/soc/1c1b000.usb/usb3/3-1\0"
                                   "ACTION=add\0"
                                   "SUBSYSTEM=usb\0"
                                   "DEVPATHkk" );
    ASSERT_THROW( UEvent{ uevent }, NetlinkManagerException );
}

TEST_F( NetlinkManagerTestSuite, ueventSourceIncorrectField ) /*1.5*/ {
    MAKE_STRING_WITH_NULL_SYMBOLS( uevent,
                                   "add@/devices/platform/soc/1c1b000.usb/usb3/3-1/\0"
                                   "ACTION=add\0"
                                   "SUBSYSTEM=usb\0"
                                   "DEVPATH=/kyky\0"
                                   "KEY-value" );
    ASSERT_THROW( UEvent{ uevent }, NetlinkManagerException );
}

TEST_F( NetlinkManagerTestSuite, ueventSourceHasUnsupportedAction ) /*1.6*/ {
    MAKE_STRING_WITH_NULL_SYMBOLS( uevent,
                                   "add@/devices/platform/soc/1c1b000.usb/usb3/3-1/\0"
                                   "ACTION=1add\0"
                                   "SUBSYSTEM=usb\0"
                                   "DEVPATH=/kyky\0"
                                   "KEY=value" );

    UEvent ue( uevent );
    ASSERT_EQ( ue.action, UEvent::Actions::UNKNOWN );
}

TEST_F( NetlinkManagerTestSuite, ueventSourceHasUnsupportedSubsystem ) /*1.7*/ {
    MAKE_STRING_WITH_NULL_SYMBOLS( uevent,
                                   "add@/devices/platform/soc/1c1b000.usb/usb3/3-1/\0"
                                   "ACTION=add\0"
                                   "SUBSYSTEM=1usb\0"
                                   "DEVPATH=/kyky\0"
                                   "KEY=value" );

    UEvent ue( uevent );
    ASSERT_EQ( ue.subsystem, UEvent::SubSystems::UNKNOWN );
}

TEST_F( NetlinkManagerTestSuite, ueventSourceIsValid ) /*1.8*/ {
    MAKE_STRING_WITH_NULL_SYMBOLS( uevent,
                                   "add@/devices/platform/soc/1c1b000.usb/usb3/3-1/\0"
                                   "ACTION=add\0"
                                   "SUBSYSTEM=usb\0"
                                   "DEVPATH=/kyky\0"
                                   "KEY=value" );

    UEvent ue( uevent );

    ASSERT_EQ( ue.action, UEvent::Actions::ADD );
    ASSERT_EQ( ue.subsystem, UEvent::SubSystems::USB );
    ASSERT_EQ( ue.devpath, "/kyky" );
    ASSERT_EQ( ue.optionalFields[ "KEY" ], "value" );
}

}  // namespace UT
}  // namespace HardwareManager
}  // namespace HubPlatform
