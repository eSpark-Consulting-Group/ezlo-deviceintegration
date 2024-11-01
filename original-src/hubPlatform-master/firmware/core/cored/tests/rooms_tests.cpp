
#include "cored/core_service.h"
#include "cored/room_manager.h"
#include "cored/remote_interface/cored_remote_interface.h"

#include "cored/tests/service/test_core_service.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

#include <memory>

namespace HubPlatform {
namespace UT {

using namespace Services;

class RoomManagerTestSuite : public ::testing::Test {
public:
    void SetUp() override {
        m_service = std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >();
        cleanDB();
        m_service->setUp();
        m_roomManager = std::make_unique< CoreD::RoomManager >( *m_service );
    }

    void TearDown() override {
        m_roomManager.reset();
        cleanDB();
        m_service->Services::Service::tearDown();
    }

    void cleanDB() {
        m_service->getDBClient().clean();
    }

public:
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes > m_service;
    std::unique_ptr< CoreD::RoomManager > m_roomManager;
};

TEST_F( RoomManagerTestSuite, roomCreateTest ) {
    {
        JsonObject request = JSON_EMPTY_OBJ;
        request[ "name" ] = "TEST_ROOM";
        const auto response = m_roomManager->createRoom( request );
        EXPECT_EQ( response[ "name" ], "TEST_ROOM" );
    }
    {
        JsonObject request = JSON_EMPTY_OBJ;
        const std::string longName = "EXTREMELY_LONG_LONG_LONG_LONG_LONG_LONG_LONG_LONG_ROOM_NAME";
        request[ "name" ] = longName;
        const auto response = m_roomManager->createRoom( request );
        EXPECT_EQ( response[ "name" ], longName.substr( 0, CoreD::RoomManager::NameMaxLength ) );
    }
}

TEST_F( RoomManagerTestSuite, roomGetTest ) {
    {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( *m_service );
        const auto response = coreDRemoteInterface.getRoom( "ROOM_ID" );
        EXPECT_TRUE( response.empty() );
    }
    {
        JsonObject request = JSON_EMPTY_OBJ;
        request[ "name" ] = "TEST_ROOM";
        const auto responseCreate = m_roomManager->createRoom( request );
        EXPECT_EQ( responseCreate[ "name" ], "TEST_ROOM" );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( *m_service );
        const auto responseGet = coreDRemoteInterface.getRoom( json_get_str( responseCreate, "_id" ) );
        EXPECT_EQ( responseGet[ "name" ], "TEST_ROOM" );
    }
}

TEST_F( RoomManagerTestSuite, roomSetNameTest ) {
    JsonObject request = JSON_EMPTY_OBJ;
    request[ "_id" ] = "ROOM_ID";
    request[ "name" ] = "ROOM_NAME";

    {
        const auto responseCreate = m_roomManager->createRoom( request );
        EXPECT_EQ( responseCreate[ "name" ], request[ "name" ] );
        const std::string id = json_get_str( responseCreate, "_id" );

        JsonObject testRequest = JSON_EMPTY_OBJ;
        testRequest[ "_id" ] = responseCreate[ "_id" ];
        testRequest[ "name" ] = "ROOM_NEW_NAME";

        const auto response = m_roomManager->setRoomName( testRequest );
        EXPECT_EQ( response[ "error" ], JSON_NULL_VAL );
    }
    {
        auto testRequest = request;

        const auto response = m_roomManager->setRoomName( testRequest );
        EXPECT_EQ( response[ "error" ][ "code" ], -32500 );
        EXPECT_EQ( response[ "error" ][ "data" ], "nma.room.notfound" );
    }
    {
        auto testRequest = request;
        testRequest.erase( "_id" );

        const auto response = m_roomManager->setRoomName( testRequest );
        EXPECT_EQ( response[ "error" ][ "code" ], -32600 );
        EXPECT_EQ( response[ "error" ][ "data" ], "rpc.params.notfound._id" );
    }
    {
        auto testRequest = request;
        testRequest.erase( "name" );

        const auto response = m_roomManager->setRoomName( testRequest );
        EXPECT_EQ( response[ "error" ][ "code" ], -32600 );
        EXPECT_EQ( response[ "error" ][ "data" ], "rpc.params.notfound.name" );
    }
    {
        auto testRequest = request;
        testRequest[ "name" ] = "";

        const auto response = m_roomManager->setRoomName( testRequest );
        EXPECT_EQ( response[ "error" ][ "code" ], -32600 );
        EXPECT_EQ( response[ "error" ][ "data" ], "rpc.params.empty.name" );
    }
}

TEST_F( RoomManagerTestSuite, roomSetOrderTest ) {
    JsonObject request = JSON_EMPTY_OBJ;
    request[ "roomsId" ] = JSON_EMPTY_ARR;

    {
        auto testRequest = request;

        const auto response = m_roomManager->setRoomOrder( testRequest );
        EXPECT_EQ( response[ "error" ], JSON_NULL_VAL );
    }
    {
        auto testRequest = request;
        testRequest.erase( "roomsId" );

        const auto response = m_roomManager->setRoomOrder( testRequest );
        EXPECT_EQ( response[ "error" ][ "code" ], -32600 );
        EXPECT_EQ( response[ "error" ][ "data" ], "rpc.params.notfound" );
    }
}

TEST_F( RoomManagerTestSuite, roomListGetTest ) {
    {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( *m_service );
        const auto responseGet = coreDRemoteInterface.getAllRooms();
        EXPECT_TRUE( responseGet.empty() );
    }
    {
        JsonObject request = JSON_EMPTY_OBJ;
        request[ "name" ] = "TEST_ROOM";
        const auto responseCreate = m_roomManager->createRoom( request );
        EXPECT_EQ( responseCreate[ "name" ], "TEST_ROOM" );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( *m_service );
        const auto responseGet = coreDRemoteInterface.getAllRooms();
        EXPECT_TRUE( responseGet.is_array() );
        EXPECT_EQ( responseGet.size(), 1 );
    }
}

TEST_F( RoomManagerTestSuite, roomsOrderTest ) {
    JsonObject request = JSON_EMPTY_OBJ;
    request[ "name" ] = "TEST_ROOM1";
    const auto responseCreate1 = m_roomManager->createRoom( request );
    EXPECT_EQ( responseCreate1[ "name" ], "TEST_ROOM1" );

    request[ "name" ] = "TEST_ROOM2";
    const auto responseCreate2 = m_roomManager->createRoom( request );
    EXPECT_EQ( responseCreate2[ "name" ], "TEST_ROOM2" );

    request[ "name" ] = "TEST_ROOM3";
    const auto responseCreate3 = m_roomManager->createRoom( request );
    EXPECT_EQ( responseCreate3[ "name" ], "TEST_ROOM3" );

    JsonObject orderRequest;
    JsonArray& arr = orderRequest[ "roomsId" ];
    arr.push_back( responseCreate3[ "_id" ] );
    arr.push_back( responseCreate2[ "_id" ] );
    arr.push_back( responseCreate1[ "_id" ] );
    m_roomManager->setRoomOrder( orderRequest );

    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( *m_service );
    auto responseGet = coreDRemoteInterface.getAllRooms();
    EXPECT_TRUE( responseGet.is_array() );
    EXPECT_EQ( responseGet.size(), arr.size() );

    EXPECT_EQ( json_get_str( responseGet[ 0 ][ "name" ] ), "TEST_ROOM3" );
    EXPECT_EQ( json_get_str( responseGet[ 1 ][ "name" ] ), "TEST_ROOM2" );
    EXPECT_EQ( json_get_str( responseGet[ 2 ][ "name" ] ), "TEST_ROOM1" );

    arr.clear();
    arr = orderRequest[ "roomsId" ];
    arr.push_back( responseCreate2[ "_id" ] );
    arr.push_back( responseCreate1[ "_id" ] );
    arr.push_back( responseCreate3[ "_id" ] );
    m_roomManager->setRoomOrder( orderRequest );

    responseGet = coreDRemoteInterface.getAllRooms();
    EXPECT_TRUE( responseGet.is_array() );
    EXPECT_EQ( responseGet.size(), arr.size() );

    EXPECT_EQ( json_get_str( responseGet[ 0 ][ "name" ] ), "TEST_ROOM2" );
    EXPECT_EQ( json_get_str( responseGet[ 1 ][ "name" ] ), "TEST_ROOM1" );
    EXPECT_EQ( json_get_str( responseGet[ 2 ][ "name" ] ), "TEST_ROOM3" );
}

TEST_F( RoomManagerTestSuite, roomDeleteTest ) {
    JsonObject request = JSON_EMPTY_OBJ;
    request[ "name" ] = "TEST_ROOM";
    const auto responseCreate = m_roomManager->createRoom( request );
    EXPECT_EQ( responseCreate[ "name" ], "TEST_ROOM" );

    m_roomManager->deleteRoom( responseCreate[ "_id" ] );

    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( *m_service );
    const auto responseGet = coreDRemoteInterface.getAllRooms();
    EXPECT_TRUE( responseGet.empty() );
}

TEST_F( RoomManagerTestSuite, roomDeleteWithOrderDbRecordTest ) {
    JsonObject request = JSON_EMPTY_OBJ;
    request[ "name" ] = "TEST_ROOM";
    const auto responseCreate = m_roomManager->createRoom( request );
    EXPECT_EQ( responseCreate[ "name" ], "TEST_ROOM" );

    JsonObject orderRequest;
    JsonArray& arr = orderRequest[ "roomsId" ];
    arr.push_back( responseCreate[ "_id" ] );
    m_roomManager->setRoomOrder( orderRequest );

    m_roomManager->deleteRoom( responseCreate[ "_id" ] );

    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( *m_service );
    const auto responseGet = coreDRemoteInterface.getAllRooms();
    EXPECT_TRUE( responseGet.empty() );

    const auto response = m_service->getDBClient().findOne( HaRoomsOrderDBName, JSON_EMPTY_OBJ, nullptr );
    EXPECT_TRUE( response[ "roomsId" ].empty() );
}

TEST_F( RoomManagerTestSuite, migrateVeraRoomsTest ) {
    auto veraModel = Json::parse( R"(
        {
            "rooms": [
                {
                    "id": 1,
                    "name": "main",
                    "section": 1
                },
                {
                    "id": 2,
                    "name": "roomA",
                    "section": 1
                },
                {
                    "id": 3,
                    "name": "roomB_LONG_LONG_LONG_LONG_LONG_LONG_LONG",
                    "section": 1
                },
                {
                    "id": 4,
                    "name": "roomC",
                    "section": 1
                }
            ]
        }
    )" );

    std::unordered_map< int, std::string > veraToEzloRoomMappings;
    m_roomManager->migrateVeraRooms( veraModel, veraToEzloRoomMappings );

    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( *m_service );
    const auto responseGet = coreDRemoteInterface.getAllRooms();

    EXPECT_EQ( responseGet.size(), 4 );

    EXPECT_EQ( responseGet[ 0 ][ "name" ], "main" );
    EXPECT_EQ( responseGet[ 0 ][ "_id" ], veraToEzloRoomMappings[ 1 ] );

    EXPECT_EQ( responseGet[ 1 ][ "name" ], "roomA" );
    EXPECT_EQ( responseGet[ 1 ][ "_id" ], veraToEzloRoomMappings[ 2 ] );

    EXPECT_EQ( responseGet[ 2 ][ "name" ], "roomB_LONG_LONG_LONG_LONG_LONG" );
    EXPECT_EQ( responseGet[ 2 ][ "_id" ], veraToEzloRoomMappings[ 3 ] );

    EXPECT_EQ( responseGet[ 3 ][ "name" ], "roomC" );
    EXPECT_EQ( responseGet[ 3 ][ "_id" ], veraToEzloRoomMappings[ 4 ] );
}

}  // namespace UT
}  // namespace HubPlatform
