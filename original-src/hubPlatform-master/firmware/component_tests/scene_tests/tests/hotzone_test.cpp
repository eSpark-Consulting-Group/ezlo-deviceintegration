#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value_type.h"
#include "cored/model/resources/cored_resources.h"
#include "database_client/dbtools.h"
#include "database_client/dbqueryparser.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"
#include "json_utils/json_wrapper.h"

#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

/*

TEST PLAN

DONE    1 any hotzone for any object
DONE    2 any hotzone for "any" filter
DONE    3 any hotzone for "anyOf" filter
DONE    4 specific hotzones for "any" filter

*/

class HotzoneTest : public SceneManagerTestsFixture {
public:
    void TearDown() override {
        SceneManagerTestsFixture::TearDown();
    }

    std::string createHotzonesItem( CoreDRemoteInterface::CoreDRemoteInterface& core, int line ) {
        auto deviceId =
            addDevice( core, "testDevice1", "camera", "vidoo_generic_cam", "vidoo_generic_cam", "camera", false, line );
        return addDictionary(
            core, deviceId, "hotzones", CoreD::Model::ValueType::Enum::CameraHotzone, true, true, line );
    }

    void updateDictionary( CoreDRemoteInterface::CoreDRemoteInterface& core,
                           const std::string& itemId,
                           const Json& value,
                           int codeLine ) {
        auto valueLoader = [&value]( HubPlatform::CoreD::Model::ValueType valueType ) {
            return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, value );
        };
        core.updateItemValue( itemId, std::move( valueLoader ) );
        checkItemUpdateBroadcast( itemId, value, codeLine );
    }

public:
    const std::string keyHotzone1 = "hotzone1";
    const std::string keyHotzone2 = "hotzone2";

    const std::string keyObjectsType = "objects";
    const std::string keyPersonsType = "persons";

    const std::string keyObjectId1 = "object1";
    const std::string keyObjectId2 = "object2";

    const std::string keyPersonId1 = "person1";
    const std::string keyPersonId2 = "person2";

public:
    class HotzonesItemValueBuilder {
    public:
        void set( const std::string& hotzoneId ) {
            using namespace HubPlatform::CoreD::Model;
            auto value = ValueFactory::createDefaultItemValue( ValueType::Enum::CameraHotzone );
            JsonObject hotzone = value->toJson();
            m_value[ hotzoneId ] = std::move( hotzone );
        }

        void set( const std::string& hotzoneId, const std::string& objectType, const std::vector< std::string > ids ) {
            namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
            if( !m_value.count( hotzoneId ) )
                set( hotzoneId );
            m_value.at( hotzoneId ).at( HotzoneFields::Detected )[ objectType ] = ids;
        }

        const JsonObject& value() {
            return m_value;
        }

    private:
        JsonObject m_value = JSON_EMPTY_OBJ;
    };
};

TEST_F( HotzoneTest, TestFixtureHotzonesBuilder ) {
    HotzonesItemValueBuilder builder;

    {  // set empty hotzone
        builder.set( keyHotzone1 );
        auto expectedJson = R"({"hotzone1":{"detected":{},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // add first hotzone
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
        auto expectedJson = R"({"hotzone1":{"detected":{"objects":["object1"]},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // add second hotzone
        builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
        auto expectedJson =
            R"({"hotzone1":{"detected":{"objects":["object1"]},"name":""},"hotzone2":{"detected":{"persons":["person1"]},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // add second object to first hotzone
        builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
        auto expectedJson =
            R"({"hotzone1":{"detected":{"objects":["object1","object2"]},"name":""},"hotzone2":{"detected":{"persons":["person1"]},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // clear first hotzone
        builder.set( keyHotzone1 );
        auto expectedJson =
            R"({"hotzone1":{"detected":{},"name":""},"hotzone2":{"detected":{"persons":["person1"]},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
    {  // clear second hotzone
        builder.set( keyHotzone2 );
        auto expectedJson = R"({"hotzone1":{"detected":{},"name":""},"hotzone2":{"detected":{},"name":""}})"_json;
        EXPECT_EQ( builder.value(), expectedJson );
    }
}

TEST_F( HotzoneTest, AnyHotzoneForAnyObject ) {  // 1
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    HotzonesItemValueBuilder builder;
    auto itemId = createHotzonesItem( coreRemoteInterface, __LINE__ );
    auto device = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto thenBlock = replaceTokensInString(
        setItemValueThenBlock, { { "__ITEM_ID__", device.m_itemId }, { "__TYPE__", "int" }, { "__VALUE__", "1" } } );

    auto isDetectedInHotzone = replaceTokensInString( isDetectedInHotzoneWhenBlockTemplate,
                                                      { { "__ITEM_ID__", itemId },
                                                        { "__HOTZONES__", R"([])" },
                                                        { "__MATCH__", "any" },
                                                        { "__FILTERS__", R"({})" } } );

    auto request = replaceTokens( createSceneRequest,
                                  { { "__NAME__", "isDetectedInHotzone" },
                                    { "__WHEN_BLOCKS__", std::move( isDetectedInHotzone ) },
                                    { "__THEN_BLOCKS__", "" } } );

    request[ "params" ][ "then" ] = JSON_ARRAY( Json::parse( thenBlock ) );

    auto sceneId = createScene( request, __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    checkItemUpdateBroadcast( device.m_itemId, 1, __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, {} );
    builder.set( keyHotzone2, keyPersonsType, {} );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( HotzoneTest, AnyHotzoneForAnyFilter ) {  // 2
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    HotzonesItemValueBuilder builder;
    auto itemId = createHotzonesItem( coreRemoteInterface, __LINE__ );

    auto isDetectedInHotzone = replaceTokensInString( isDetectedInHotzoneWhenBlockTemplate,
                                                      { { "__ITEM_ID__", itemId },
                                                        { "__HOTZONES__", R"([])" },
                                                        { "__MATCH__", "filter" },
                                                        { "__FILTERS__", R"({"objects":{"match":"any"}})" } } );

    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isDetectedInHotzone" },
                                          { "__WHEN_BLOCKS__", std::move( isDetectedInHotzone ) },
                                          { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    builder.set( keyHotzone1, keyPersonsType, { keyPersonId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, {} );
    builder.set( keyHotzone2, keyPersonsType, {} );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyObjectsType, { keyObjectId2 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( HotzoneTest, AnyHotzoneForAnyOfFilter ) {  // 3
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    HotzonesItemValueBuilder builder;
    auto itemId = createHotzonesItem( coreRemoteInterface, __LINE__ );

    auto filterJson = JSON( "objects" << JSON( "match"
                                               << "anyOf"
                                               << "list" << JSON_ARRAY( keyObjectId1 << keyObjectId2 ) ) );
    auto isDetectedInHotzone = replaceTokensInString( isDetectedInHotzoneWhenBlockTemplate,
                                                      { { "__ITEM_ID__", itemId },
                                                        { "__HOTZONES__", R"([])" },
                                                        { "__MATCH__", "filter" },
                                                        { "__FILTERS__", filterJson.dump() } } );

    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isDetectedInHotzone" },
                                          { "__WHEN_BLOCKS__", std::move( isDetectedInHotzone ) },
                                          { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    builder.set( keyHotzone1, keyPersonsType, { keyPersonId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, {} );
    builder.set( keyHotzone2, keyPersonsType, {} );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyObjectsType, { keyObjectId2 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

TEST_F( HotzoneTest, SpecificHotzoneForAnyFilter ) {  // 4
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    HotzonesItemValueBuilder builder;
    auto itemId = createHotzonesItem( coreRemoteInterface, __LINE__ );

    auto isDetectedInHotzone = replaceTokensInString( isDetectedInHotzoneWhenBlockTemplate,
                                                      { { "__ITEM_ID__", itemId },
                                                        { "__HOTZONES__", "[\"" + keyHotzone1 + "\"]" },
                                                        { "__MATCH__", "filter" },
                                                        { "__FILTERS__", R"({"objects":{"match":"any"}})" } } );

    const auto request = replaceTokens( createSceneRequest,
                                        { { "__NAME__", "isDetectedInHotzone" },
                                          { "__WHEN_BLOCKS__", std::move( isDetectedInHotzone ) },
                                          { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request, __LINE__ );

    builder.set( keyHotzone1, keyPersonsType, { keyPersonId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyObjectsType, { keyObjectId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId1, keyObjectId2 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, {} );
    builder.set( keyHotzone2, keyPersonsType, {} );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyPersonsType, { keyPersonId1 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone2, keyObjectsType, { keyObjectId2 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    builder.set( keyHotzone1, keyObjectsType, { keyObjectId2 } );
    updateDictionary( coreRemoteInterface, itemId, builder.value(), __LINE__ );

    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
