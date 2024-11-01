#include "zigbee/interview_configurations/node_interview_names_extractors.h"

#include "json_utils/json_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <regex>

namespace HubPlatform {
namespace Zigbee {
namespace UT {
using namespace ::testing;

class ZigbeeInterviewNamesExtractorTest : public Test {};

TEST_F( ZigbeeInterviewNamesExtractorTest, testNotCreated_1 ) {
    auto config = JSON( "devices_configurations" << JSON_EMPTY_ARR );
    auto obj = NodeInterviewNamesExtractors::create( config );
    EXPECT_EQ( obj.get(), nullptr );
}

TEST_F( ZigbeeInterviewNamesExtractorTest, testCreatedEmpty_2 ) {
    auto obj1 = NodeInterviewNamesExtractors::create( JSON( "manufacturer_name_extractors" << JSON_EMPTY_ARR ) );
    EXPECT_NE( obj1.get(), nullptr );

    auto obj2 = NodeInterviewNamesExtractors::create( JSON( "model_name_extractors" << JSON_EMPTY_ARR ) );
    EXPECT_NE( obj2.get(), nullptr );

    auto obj3 = NodeInterviewNamesExtractors::create(
        JSON( "model_name_extractors" << JSON_EMPTY_ARR << "manufacturer_name_extractors" << JSON_EMPTY_ARR ) );
    EXPECT_NE( obj2.get(), nullptr );
}

TEST_F( ZigbeeInterviewNamesExtractorTest, testModelExtractor_3 ) {
    auto config = JSON( "model_name_extractors" << JSON_ARRAY( JSON( "regex"
                                                                     << "MaestroStat.*"
                                                                     << "name"
                                                                     << "MaestroStat" )
                                                               << JSON( "regex"
                                                                        << "STZB402.*"
                                                                        << "name"
                                                                        << "STZB402" ) ) );
    auto obj = NodeInterviewNamesExtractors::create( config );
    ASSERT_NE( obj.get(), nullptr );

    // check first suites
    EXPECT_EQ( obj->getExtractedModelName( "MaestroStat5F" ).has_value(), true );
    EXPECT_EQ( *obj->getExtractedModelName( "MaestroStat5F" ), std::string( "MaestroStat" ) );

    // check second suites
    EXPECT_EQ( obj->getExtractedModelName( "STZB402@dcsdc6" ).has_value(), true );
    EXPECT_EQ( *obj->getExtractedModelName( "STZB402@dcsdc6" ), std::string( "STZB402" ) );

    // check first not suites
    EXPECT_EQ( obj->getExtractedModelName( "@5MaestroStat5F" ).has_value(), false );

    // check second suites
    EXPECT_EQ( obj->getExtractedModelName( "FsSTZB402@dcsdc6" ).has_value(), false );
}

TEST_F( ZigbeeInterviewNamesExtractorTest, testManufacturerExtractor_4 ) {
    auto config = JSON( "manufacturer_name_extractors" << JSON_ARRAY( JSON( "regex"
                                                                            << "MaestroStat.*"
                                                                            << "name"
                                                                            << "MaestroStat" )
                                                                      << JSON( "regex"
                                                                               << "STZB402.*"
                                                                               << "name"
                                                                               << "STZB402" ) ) );
    auto obj = NodeInterviewNamesExtractors::create( config );
    ASSERT_NE( obj.get(), nullptr );

    // check first suites
    EXPECT_EQ( obj->getExtractedManufacturerName( "MaestroStat5F" ).has_value(), true );
    EXPECT_EQ( *obj->getExtractedManufacturerName( "MaestroStat5F" ), std::string( "MaestroStat" ) );

    // check second suites
    EXPECT_EQ( obj->getExtractedManufacturerName( "STZB402@dcsdc6" ).has_value(), true );
    EXPECT_EQ( *obj->getExtractedManufacturerName( "STZB402@dcsdc6" ), std::string( "STZB402" ) );

    // check first not suites
    EXPECT_EQ( obj->getExtractedManufacturerName( "@5MaestroStat5F" ).has_value(), false );

    // check second suites
    EXPECT_EQ( obj->getExtractedManufacturerName( "FsSTZB402@dcsdc6" ).has_value(), false );
}

}  // namespace UT
}  // namespace Zigbee
}  // namespace HubPlatform
