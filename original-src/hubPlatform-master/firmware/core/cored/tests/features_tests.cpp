#include "cored/core_service.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/tests/service/test_core_service.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "cored/features_manager.h"
#include "database_client/dbqueryparser.h"

#include <gtest/gtest.h>

/*
TESTPLAN

DONE    1. Check protect_delayed feature
DONE    2. Check protect feature status
DONE        2.1 Check webrtc feature status
DONE    3. Check extra features
DONE    4. External feature adding
DONE    5. External feature removing
DONE    6. Invalid custom feature data
DONE    7. Missing custom feature data
DONE    8. valid file json data parse
DONE    9. External feature updating

*/

namespace HubPlatform {
namespace UT {

using namespace Services;
using namespace CoreD::Model::Resources;

class FeatureManagerTestSuite : public ::testing::Test {
public:
    using KeyJsonList = std::vector< std::pair< std::string, JsonObject > >;

public:
    void SetUp() override;
    Service& getService();
    IDbClient& getDBClient();
    void cleanDB();
    Services::UT::FakeResponseBus& getFakeResponseBus();

public:
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes > m_service;
    MessageSubClasses m_msgClass = UNKNOWN;
};

Service& FeatureManagerTestSuite::getService() {
    return *m_service;
}

IDbClient& FeatureManagerTestSuite::getDBClient() {
    return getService().getDBClient();
}

void FeatureManagerTestSuite::cleanDB() {
    getDBClient().clean();
}

void FeatureManagerTestSuite::SetUp() {
    m_service =
        std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >( std::make_unique< Services::BusWrapper >(
            Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >() ) );

    getDBClient().clean();
    m_service->setUp();
}

Services::UT::FakeResponseBus& FeatureManagerTestSuite::getFakeResponseBus() {
    return *dynamic_cast< Services::UT::FakeResponseBus* >( m_service->getBus() );
}

Json featureData( const std::string& name,
                  const std::string& status,
                  bool editable,
                  const std::string& version = "1.0" ) {
    return JSON( "service"
                 << "external"
                 << "custom_features"
                 << JSON_ARRAY( JSON( "name" << name << "status" << status << "version" << version << "editable"
                                             << editable ) ) );
}

TEST_F( FeatureManagerTestSuite, checkProtectDelayedFeature ) /* 1 */ {
    using namespace CoreD;
    const char* const featureProtectDelayed = "protect_delayed";
    ExternalFeaturesManager features( getService() );
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto feature = coreDInterface.getFeatures();
    auto featurePtr = coreDInterface.findFeature( featureProtectDelayed );

    ASSERT_TRUE( featurePtr.get() != nullptr );
    ASSERT_TRUE( coreDInterface.checkFeatureStatus( featureProtectDelayed ) );
    ASSERT_TRUE( featurePtr->getStatus() );
    ASSERT_FALSE( featurePtr->isEditable() );
}

TEST_F( FeatureManagerTestSuite, checkProtectFeatureStatus ) /* 2 */ {
    using namespace CoreD;
    const char* const featureProtect = "protect";
    const bool statusExpected = false;
    const bool editable = true;
    const bool isCustomExpected = false;

    ExternalFeaturesManager features( getService() );
    getDBClient().drop( DBNames::FeaturesDB );

    ControllerFeature feature( "protect", { 1, 0 }, statusExpected, editable, isCustomExpected );
    getDBClient().insert( DBNames::FeaturesDB, feature.toDbRecord() );

    CoreDRemoteInterface::CoreDRemoteInterface core( getService() );
    auto featurePtr = core.findFeature( featureProtect );

    ASSERT_TRUE( featurePtr.get() != nullptr );
    ASSERT_EQ( core.checkFeatureStatus( featureProtect ), statusExpected );
    ASSERT_EQ( featurePtr->isEditable(), editable );
    ASSERT_FALSE( featurePtr->isCustom() );
}

TEST_F( FeatureManagerTestSuite, checkWebrtcFeatureStatus ) /* 2.1 */ {
    using namespace CoreD;
    const char* const featureProtect = "webrtc";

#ifdef __HUB_WITH_WEBRTC
    auto makeExpectedStatus = []() { return true; };
    auto checkExpectedValue = [&]( bool featureStatus, const char* file, int codeLine ) {
        ASSERT_EQ( featureStatus, makeExpectedStatus() ) << "At: " << file << ":" << codeLine;
    };
#else
    auto makeExpectedStatus = []() -> bool { return false; };
    auto checkExpectedValue = [&]( bool featureStatus, const char* file, int codeLine ) {
        ASSERT_EQ( featureStatus, makeExpectedStatus() ) << "At: " << file << ":" << codeLine;
    };
#endif

    // init: webrtc: editable, status true
    const bool statusExpected = true;
    const bool editable = true;
    const bool isCustom = false;

    ControllerFeature feature( "webrtc", { 1, 0 }, statusExpected, editable, isCustom );
    getDBClient().insert( DBNames::FeaturesDB, feature.toDbRecord() );

    // after modification: non-editable, status false if there is no WITH_WEBRTC
    ExternalFeaturesManager features( getService() );
    CoreDRemoteInterface::CoreDRemoteInterface core( getService() );
    auto featurePtr = core.findFeature( featureProtect );

    ASSERT_TRUE( featurePtr.get() != nullptr );
    const bool featureStatus = core.checkFeatureStatus( featureProtect );
    checkExpectedValue( featureStatus, __FILE__, __LINE__ );
    ASSERT_NE( featurePtr->isEditable(), editable );
    ASSERT_EQ( featurePtr->isCustom(), isCustom );
}

TEST_F( FeatureManagerTestSuite, checkExtraFeatures ) /* 3 */ {
    using namespace CoreD;
    const char* const Test1 = "feature1";
    const char* const Test2 = "feature2";
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( db::ID_FIELD_NAME << Test1 << "version"
                                                  << "1.0"
                                                  << "status"
                                                  << "false"
                                                  << "editable"
                                                  << "false"
                                                  << "is_custom" << false ) );
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( db::ID_FIELD_NAME << Test2 << "version"
                                                  << "1.0"
                                                  << "status"
                                                  << "true"
                                                  << "editable"
                                                  << "false"
                                                  << "is_custom" << false ) );

    ExternalFeaturesManager features( getService() );
    CoreDRemoteInterface::CoreDRemoteInterface core( getService() );
    auto featurePtr1 = core.findFeature( Test1 );
    auto featurePtr2 = core.findFeature( Test2 );

    ASSERT_TRUE( featurePtr1.get() == nullptr );
    ASSERT_TRUE( featurePtr2.get() == nullptr );
}

TEST_F( FeatureManagerTestSuite, externalPackageAdding ) /* 4 */ {
    using namespace CoreD;
    const char* const externalFeature = "external.feature1";
    ExternalFeaturesManager features( getService() );
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto feature = coreDInterface.getFeatures();
    auto featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() == nullptr );
    bool editable = false;
    auto data = featureData( "feature1", "on", editable );
    features.processPackageAdding( data );
    featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() != nullptr );
    ASSERT_EQ( featurePtr->getName(), externalFeature );
    ASSERT_TRUE( featurePtr->getStatus() );
    ASSERT_FALSE( featurePtr->isEditable() );

    auto message = getFakeResponseBus().getMessage();

    EXPECT_FALSE( message.group.empty() );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_FEATURE_ADDED );
    EXPECT_FALSE( message.data.empty() );

    EXPECT_EQ( message.data[ "_id" ], externalFeature );
    EXPECT_EQ( message.data[ "editable" ], false );
    EXPECT_EQ( message.data[ "status" ], true );
    EXPECT_EQ( message.data[ "version" ], "1.0" );
}

TEST_F( FeatureManagerTestSuite, externalPackageRemoving ) /* 5 */ {
    using namespace CoreD;
    std::string externalFeature = "external.feature1";
    ExternalFeaturesManager features( getService() );
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto feature = coreDInterface.getFeatures();
    auto featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() == nullptr );

    bool editable = false;
    auto data = featureData( "feature1", "on", editable );

    features.processPackageAdding( data );
    featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() != nullptr );
    ASSERT_EQ( featurePtr->getName(), externalFeature );
    auto isPresent = HubPlatform::CoreD::ExternalFeaturesManager::processPackageRemoving( data, externalFeature );
    ASSERT_TRUE( isPresent );
    data = featureData( "feature0", "on", editable );
    isPresent = HubPlatform::CoreD::ExternalFeaturesManager::processPackageRemoving( data, externalFeature );
    ASSERT_FALSE( isPresent );
    features.removeCustomFeature( externalFeature );

    auto message = getFakeResponseBus().getMessage();

    EXPECT_FALSE( message.group.empty() );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_FEATURE_REMOVED );
    EXPECT_FALSE( message.data.empty() );

    EXPECT_EQ( message.data[ "_id" ], externalFeature );
}

TEST_F( FeatureManagerTestSuite, invalidJsonFileContent ) /* 6 */ {
    using namespace CoreD;

    bool editable = false;
    auto data = featureData( "feature1", "disable", editable );

    JsonArray customFeatures;
    json_get_arr( data, "custom_features", customFeatures );

    ControllerFeature feature;

    ASSERT_THROW( feature.checkFeature( customFeatures[ 0 ] ), ControllerFeatureCheckError )
        << "Field 'status' invalid value";
    customFeatures[ 0 ][ "status" ] = "on";
    customFeatures[ 0 ][ "version" ] = "one";

    ASSERT_THROW( feature.checkFeature( customFeatures[ 0 ] ), ControllerFeatureCheckError )
        << "Field 'version' invalid value";

    customFeatures[ 0 ][ "version" ] = 1.0;

    ASSERT_THROW( feature.checkFeature( customFeatures[ 0 ] ), ControllerFeatureCheckError )
        << "Field 'version' invalid type";

    customFeatures[ 0 ][ "version" ] = "1.0";
    customFeatures[ 0 ][ "editable" ] = "false";

    ASSERT_THROW( feature.checkFeature( customFeatures[ 0 ] ), ControllerFeatureCheckError )
        << "Field 'editable' not a boolean type";
    ASSERT_THROW( feature.checkFeature( data[ "service" ] ), ControllerFeatureCheckError )
        << "custom_features array keys not found";
}

TEST_F( FeatureManagerTestSuite, missingFileContents ) /* 7 */ {
    using namespace CoreD;
    const char* const externalFeature = "external.feature1";
    ExternalFeaturesManager features( getService() );
    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto feature = coreDInterface.getFeatures();
    auto featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() == nullptr );

    auto data = JSON( "custom_features" << JSON_ARRAY( JSON( "name"
                                                             << "feature1"
                                                             << "status"
                                                             << "on"
                                                             << "version"
                                                             << "1.0"
                                                             << "editable" << false ) ) );

    features.processPackageAdding( data );
    featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() == nullptr ) << "Custom feature required field not found: service";

    data = JSON( "service"
                 << "external" );

    features.processPackageAdding( data );
    featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() == nullptr ) << "Custom features not found";
}

TEST_F( FeatureManagerTestSuite, validFileJsonDataParsing ) /* 8 */ {
    using namespace CoreD;
    bool editable = false;
    auto data = featureData( "feature1", "on", editable );
    JsonArray customFeatures;
    auto baseName = json_get_str( data, "service" );
    json_get_arr( data, "custom_features", customFeatures );

    ControllerFeature feature;
    ASSERT_TRUE( feature.fromJson( baseName, customFeatures[ 0 ] ) );
    EXPECT_EQ( feature.name, "external.feature1" );
    EXPECT_EQ( feature.status, true );
    EXPECT_EQ( feature.editable, false );
    EXPECT_EQ( feature.version.toString(), "1.0" );
}

TEST_F( FeatureManagerTestSuite, externalPackageUpdating ) /* 9 */ {
    using namespace CoreD;
    const char* const externalFeature = "external.feature1";
    ExternalFeaturesManager features( getService() );

    CoreDRemoteInterface::CoreDRemoteInterface coreDInterface( getService() );
    auto feature = coreDInterface.getFeatures();
    auto featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() == nullptr );
    bool editable = true;
    auto newFeature = featureData( "feature1", "off", editable );
    features.processPackageAdding( newFeature );
    auto message = getFakeResponseBus().getMessage();
    auto updateFeature = featureData( "feature1", "on", true, "2.0" );
    features.processPackageAdding( updateFeature );
    featurePtr = coreDInterface.findFeature( externalFeature );
    ASSERT_TRUE( featurePtr.get() != nullptr );
    ASSERT_EQ( featurePtr->getName(), externalFeature );
    ASSERT_TRUE( featurePtr->getStatus() );
    ASSERT_TRUE( featurePtr->isEditable() );

    message = getFakeResponseBus().getMessage();

    EXPECT_FALSE( message.group.empty() );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_FEATURE_UPDATED );
    EXPECT_FALSE( message.data.empty() );

    EXPECT_EQ( message.data[ "_id" ], externalFeature );
    EXPECT_EQ( message.data[ "status" ], true );
    EXPECT_EQ( message.data[ "version" ], "2.0" );
}

}  // namespace UT
}  // namespace HubPlatform