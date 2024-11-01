#pragma once

#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace Tests {

class SceneManagerTestsFixture : public CoreNmaTest {
public:
    using SceneID = std::string;

public:
    void SetUp() override;
    void TearDown() override;

    JsonObject sendRequestAndCheckAnswer( const Json& request, int codeLine );

    JsonObject sendRequestAndCheckFailedAnswer( const JsonObject& request, int codeLine );
    JsonObject sendRequestAndCheckFailedAnswer( const JsonObject& request,
                                                Errors::Error expectedError,
                                                const std::string& expectedMessage,
                                                int codeLine );

    SceneID createScene( const JsonObject& sceneJson, int codeLine = -1 );
    SceneID createScene( const JsonObject& sceneJson, JsonObject& result, int codeLine = -1 );
    void editScene( const std::string& sceneId, const JsonObject& sceneJson, int codeLine = -1 );
    SceneID cloneScene( const JsonObject& request, JsonObject& resultFromBroadcast, int codeLine );

    void testRunProgressBroadcast( const JsonObject& createJson,
                                   const std::string& sceneId,
                                   const std::string& status,
                                   int codeLine,
                                   const JsonArray& notifications = JSON_EMPTY_ARR,
                                   std::chrono::seconds timeoutSeconds = progressBroadcastTimeout );

    void testRunProgressBroadcast( const JsonObject& broadcast,
                                   const JsonObject& createJson,
                                   const std::string& sceneId,
                                   const std::string& status,
                                   int codeLine,
                                   const JsonArray& notifications = JSON_EMPTY_ARR );

    bool isRunProgressBroadcast( const JsonObject& broadcast,
                                 const JsonObject& createJson,
                                 const std::string& sceneId,
                                 const std::string& status );

    bool isRunProgressBroadcast( const JsonObject& broadcast, const std::string& sceneId, const std::string& status );

    bool isEnableChangedBroadcast( const JsonObject& broadcast, const std::string& sceneId, bool isEnabled );

    void deleteScenes();
    void deleteScene( const SceneID& sceneId, int codeLine = -1 );

    void runSceneManually( SceneID sceneId, int codeLine );
    void runSceneManually( SceneID sceneId, const std::string& block, int codeLine );

    void checkChangedBroadcast( SceneManagerTestsFixture::SceneID sceneId, const JsonObject& expected, int codeLine );
    void checkChangedBroadcast( SceneManagerTestsFixture::SceneID sceneId, int codeLine );
    void checkChangedBroadcastEditRequest( SceneID sceneId, const JsonObject& editRequest, int codeLine );
    void checkChangedBroadcastEnableRequest( SceneID sceneId, const JsonObject& scene, bool enable, int codeLine );

    bool waitAndCheckRunProgressBroadcast( const JsonObject& createJson,
                                           const std::string& sceneId,
                                           const std::string& status,
                                           int codeLine );

    void checkErrorFieldNotFound( const JsonObject& answer, const std::string& fieldName, int codeLine );
    void checkErrorEmptyField( const JsonObject& answer, const std::string& fieldName, int codeLine );

    std::string hostName();
    unsigned& getPort();
    std::string makeHostName();
    std::string makeUrl( const char* path, const char* hostName = "", const char* proto = "" );

    void testIsSceneStateResult( const std::string& result );

    void removeBlockIds( Json& blocks );

    void sendNetworkChangedBroadcast( const std::string& interface, bool isInternetAvailable );

    static void setGeoCoordinates();

    static JsonObject getSunStateRequest( const std::string& type,
                                          const std::string& sunState,
                                          const std::string& when,
                                          const std::string& rangeType = "at",
                                          const std::string& time = "01:33:14" );

    void enableSceneBlock( const std::string& sceneId, const std::string& blockId, bool enabled, int codeLine );

    void testBatteryLevelWithComparator( const std::string& comparator, int level1, int level2, int codeLine );

    SceneID sceneWithBlocks( int codeLine );

    JsonObject getItemValue( const std::string itemId );

    JsonObject getSceneStatus( const std::string& sceneId, int codeLine );

    std::string createSecuritySensorDevice( const std::string& deviceName, const std::string& subcategory );

    template < typename ValueType >
    bool waitAndCheckItemUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::ItemID itemId,
                                          ValueType value,
                                          int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );

        auto broadcast = waitForBroadcast( "hub.item.updated", codeLineStr );
        if( json_get_str( broadcast, "msg_subclass" ) == "hub.item.updated" ) {
            JsonObject result;
            json_get_obj( broadcast, "result", result );
            if( json_get_str( result, "_id" ) == itemId ) {
                auto res = result[ "value" ];
                LOGGER_TRACE( "res: " + cc::j( res ) );
                if( isValueEqual( value, result[ "value" ], codeLineStr ) )
                    return true;
            }
        }
        return false;
    }

    bool waitAndCheckItemUpdateBroadcast( CoreDRemoteInterface::CoreDInterface::ItemID itemId, int codeLine ) {
        std::string codeLineStr = getCodeLineString( codeLine );
        auto broadcast = waitForBroadcast( "hub.item.updated", codeLineStr );

        if( json_get_str( broadcast, "msg_subclass" ) == "hub.item.updated" ) {
            JsonObject result;
            json_get_obj( broadcast, "result", result );
            if( json_get_str( result, "_id" ) == itemId ) {
                return true;
            }
        }
        return false;
    }

protected:
    void deleteScene( std::set< SceneID >& container,
                      const SceneManagerTestsFixture::SceneID& sceneId,
                      const std::string& methodName,
                      const std::string& broadcastName,
                      int codeLine );

    JsonArray getOptionValues( const JsonObject& block ) const;

protected:
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    std::deque< JsonObject > m_broadcastMessages;
    std::set< SceneID > m_createdScenes;
    unsigned int m_port;
};

}  // namespace Tests
}  // namespace HubPlatform
