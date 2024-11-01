#include "scenes_manager/scenes_engine/events/firmware_update_state_event.h"
#include "scenes_manager/scenes_engine/events/event_base.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

class FirmwareUpdateStateEventTests : public testing::Test {
public:
    static bool testFirmwareUpdateState( std::string updateManagerState, std::string conditionBlockState ) {
        FirmwareUpdateStateEvent event( conditionBlockState );
        return event.compare( updateManagerState );
    }
};

TEST_F( FirmwareUpdateStateEventTests, testCompareBetween ) {
    ASSERT_TRUE( testFirmwareUpdateState( "started", "started" ) );
    ASSERT_TRUE( testFirmwareUpdateState( "process", "updating" ) );
    ASSERT_TRUE( testFirmwareUpdateState( "finished", "done" ) );

    ASSERT_FALSE( testFirmwareUpdateState( "started", "done" ) );
    ASSERT_FALSE( testFirmwareUpdateState( "started", "updating" ) );
    ASSERT_FALSE( testFirmwareUpdateState( "process", "started" ) );
    ASSERT_FALSE( testFirmwareUpdateState( "process", "done" ) );
}

}  // namespace UT
}  // namespace SceneEngine
