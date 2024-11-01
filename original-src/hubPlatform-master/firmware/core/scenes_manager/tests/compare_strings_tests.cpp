#include "scenes_manager/scenes_engine/events/compare_strings_event.h"
#include "test_utils/temporary_root_folder_wrapper.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

class CompareStringsEventTest : public ::testing::Test {};

TEST_F( CompareStringsEventTest, equal ) {
    CompareStringsEvent event( "50", std::equal_to< CompareStringsEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
}

TEST_F( CompareStringsEventTest, notEqual ) {
    CompareStringsEvent event( "50", std::not_equal_to< CompareStringsEvent::ItemValue >() );
    EXPECT_TRUE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "50" } ) );
    EXPECT_TRUE( event.compare( { "51" } ) );
}

TEST_F( CompareStringsEventTest, greater ) {
    CompareStringsEvent event( "50", std::greater< CompareStringsEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { "48" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "50" } ) );
    EXPECT_TRUE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "52" } ) );
    EXPECT_FALSE( event.compare( { "53" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "50" } ) );
    EXPECT_TRUE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "52" } ) );
}

TEST_F( CompareStringsEventTest, greaterEqual ) {
    CompareStringsEvent event( "50", std::greater_equal< CompareStringsEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { "48" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "52" } ) );
    EXPECT_FALSE( event.compare( { "53" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "52" } ) );
}

TEST_F( CompareStringsEventTest, less ) {
    CompareStringsEvent event( "50", std::less< CompareStringsEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { "52" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "50" } ) );
    EXPECT_TRUE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "48" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "50" } ) );
    EXPECT_TRUE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "48" } ) );
}

TEST_F( CompareStringsEventTest, lessEqual ) {
    CompareStringsEvent event( "50", std::less_equal< CompareStringsEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { "52" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "48" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_FALSE( event.compare( { "48" } ) );
}

TEST_F( CompareStringsEventTest, withoutState ) {
    CompareStringsEvent event( "50", std::equal_to< CompareStringsEvent::ItemValue >() );
    event.setKeepState( false );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
    EXPECT_FALSE( event.compare( { "51" } ) );
    EXPECT_FALSE( event.compare( { "49" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
    EXPECT_TRUE( event.compare( { "50" } ) );
}

}  // namespace UT
}  // namespace SceneEngine
