#include "networkd/ssh_tunnel/ssh_tunnel.h"
#include "networkd/ssh_tunnel/ssh_camera_tunnel.h"
#include "networkd/ssh_tunnel/ssh_support_tunnel.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include "service_interfaces/tests/fixtures/fake_service_fixture.h"
#include "provision_data_utils/provision_data_manager.h"

#include "common_utils/cpp14.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

/*

    TEST PLAN
DONE    1. Init tunnels with no data in database
DONE    2. Start tunnels from database
DONE        2.1 Start camera tunnels
DONE        2.2 Start support tunnels
DONE        2.3 Start disabled support tunnels
DONE    3. Send token request
DONE    4. Invalid cloud response for token request
DONE    5. Start tunnels
DONE        5.1 Start single tunnel
DONE        5.2 Start triple tunnels
DONE        5.3 Start tunnel and add new tunnel
DONE    6. Remove tunnels
DONE        6.1 Remove tunnel in Created state
DONE        6.2 Remove last tunnel
DONE        6.3 Remove tunnel in Request token state
DONE        6.4 Remove and create tunnel in Request token state variant 1
DONE        6.5 Remove and create tunnel in Request token state variant 2
DONE        6.6 Remove tunnel in Stop token state variant 1
DONE        6.7 Remove tunnel in Stop token state variant 2
DONE        6.8 Remove tunnel in Stop token state is timed out
DONE`       6.9 Remove and create tunnel is Created state
DONE    7. Tunnels are updated during stopping
DONE    8. Tunnel stopping timeout
DONE    9. Received less ssh connections than tunnels
DONE    10. Tunnels are updated during token request
DONE    11. Token is expired
DONE    12. Timeout of token request
DONE    13. Tunnels updating during monitoring
DONE    14. Tunnels was closed or rejected
DONE        14.1 Tunnel is rejected
DONE        14.2 Tunnel is closed by unknown reason
DONE        14.3 Tunnel is closed
DONE    15. Random balancing if received endpoint les than tunnels

 */

namespace HubPlatform {

namespace Ssh {
bool operator==( const TunnelManager::Data& a, const TunnelManager::Data& b ) {
    return a.ownPort == b.ownPort && a.remotePort == b.remotePort && a.serverAddress == b.serverAddress &&
           a.serverPass == b.serverPass && a.serverPort == b.serverPort && a.serverUser == b.serverUser &&
           a.keepAliveTimeoutSec == b.keepAliveTimeoutSec;
}

std::ostream& operator<<( std::ostream& os, const TunnelManager::Data& data ) {
    os << "ownPort: " << data.ownPort << ", remotePort: " << data.remotePort
       << ", serverAddress: " << data.serverAddress << ", serverPass: " << data.serverPass
       << ", serverPort: " << data.serverPort << ", serverUser: " << data.serverUser
       << ", keepAliveTimeoutSec: " << data.keepAliveTimeoutSec;
    return os;
}

}  // namespace Ssh

namespace Network {
namespace SshTunnel {
namespace UT {

using testing::_;
using testing::ByRef;
using testing::DoAll;
using testing::InvokeArgument;
using testing::Return;
using testing::UnorderedElementsAreArray;

class TunnelManagerMock final : public TunnelManager {
public:
    MOCK_METHOD2( createSshTunnel, pid_t( const Data& data, OnCloseSshTunnel OnCloseSshTunnel ) );
    MOCK_METHOD1( killSshTunnel, void( pid_t processId ) );
};

class FakeServiceTestFixture : public Services::UT::ServiceFixture, public Services::UT::FakeBridgeService {
public:
    void SetUp() override {
        createDatabase();
        Services::UT::ServiceFixture::SetUp();
    }

    FakeBridgeService& getHostService() final override {
        return *this;
    }
    Services::UT::FakeResponseBridgeBus& getFakeBus() {
        return *dynamic_cast< Services::UT::FakeResponseBridgeBus* >( getBus() );
    }
};

class SshTunnelTests : public FakeServiceTestFixture {
public:
    void waitTimer( const std::chrono::seconds& timeout ) {
        sleep( timeout );
        waitForTasks();
    }

    void sendCloudRespose( Services::UT::FakeBridgeService& fakeService,
                           const JsonArray& identities,
                           const std::vector< std::string >& endpoints,
                           int codeLine ) {
        auto message = getFakeBus().getMessage();
        checkTokenRequest( message, identities, JSON_EMPTY_ARR, codeLine );
        auto sender = json_get_str( message.data, "sender" );
        auto uuid = json_get_str( message.data, "uuid" );
        auto response = getTokenResponse( endpoints );
        fakeService.sendAsyncResponse( sender, response, uuid );
    }

    void checkTokenRequest( const Services::Message& message,
                            const JsonArray& identities,
                            const JsonArray& rejected,
                            int codeLine ) {
        auto codeLineStr = "code line: " + std::to_string( codeLine );
        auto& requestBody = message.data[ "requestBody" ];
        EXPECT_EQ( message.group, GroupsNames::UI_GROUP ) << codeLineStr;
        EXPECT_EQ( message.msgClass, REQUEST ) << codeLineStr;
        EXPECT_EQ( message.msgSubClass, CLOUD_MESSAGE ) << codeLineStr;
        EXPECT_EQ( json_get_str( requestBody, "method" ), "cloud.tunnel_camera_coordinator" ) << codeLineStr;
        EXPECT_EQ( json_get_str( requestBody, "api" ), "1" ) << codeLineStr;
        JsonArray identitiesArray;
        for( const auto& identity : identities ) {
            identitiesArray.push_back( JSON( "identity" << identity ) );
        }
        EXPECT_EQ( requestBody[ "params" ], JSON( "forwards" << identities << "reject" << rejected ) ) << codeLineStr;
    }

    void checkDbData( Json expectedDbData, size_t expectedProcessIds, int codeLine ) {
        auto codeLineStr = "code line: " + std::to_string( codeLine );
        auto& dbClient = getHostService().getDBClient();
        const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
        auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
        ASSERT_EQ( dbData.size(), 1 ) << codeLineStr;
        EXPECT_THAT( dbData[ 0 ][ "tunnels" ], UnorderedElementsAreArray( expectedDbData[ "tunnels" ] ) )
            << codeLineStr;
        auto& provision = ProvisionDataUtils::getManager().getProvisionData();
        dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
        dbData[ 0 ].erase( "_id" );
        dbData[ 0 ].erase( "status" );
        dbData[ 0 ].erase( "tunnels" );
        expectedDbData.erase( "tunnels" );
        EXPECT_EQ( dbData[ 0 ], expectedDbData ) << codeLineStr;
        dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
        ASSERT_EQ( dbData.size(), expectedProcessIds ) << codeLineStr;
    }

    Json getTokenResponse( const std::vector< std::string >& links, int expiration = 2147483647 ) const {
        auto token = JSON( "token" << JSON( "string"
                                            << "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"
                                            << "expires_ts" << expiration ) );
        return JSON( token << "endpoints" << JSON( "ssh" << links ) );
    }

    Json getDbData( const std::vector< TunnelData >& tunnelsData ) const {
        JsonArray tunnels = JSON_EMPTY_ARR;
        for( const auto& tunnel : tunnelsData ) {
            tunnels.push_back( JSON( "tunnelLink" << tunnel.data.serverAddress << "tunnelPort" << tunnel.data.serverPort
                                                  << "remotePort" << tunnel.data.remotePort << "localPort"
                                                  << tunnel.data.ownPort << "identity" << tunnel.identity ) );
        }
        return JSON( "method"
                     << "cloud.tunnel_camera_coordinator"
                     << "token"
                     << "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"
                     << "tokenExpirationDate" << 2147483647 << "tunnels" << tunnels );
    }

    State& getState( SshTunnel& sshTunnel ) {
        return *sshTunnel.m_state;
    }

    SshTunnel::Tunnels& getTunnels( SshTunnel& sshTunnel ) {
        return sshTunnel.m_tunnels;
    }

    int getKeepAliveTimeout() const {
        return SshTunnel::KeepAliveTimeout.count();
    }

    std::chrono::seconds getTimeout( SshTunnel& sshTunnel ) const {
        return sshTunnel.getTimeout();
    }

    TunnelsData getTunnelsData( SshTunnel& sshTunnel,
                                const Json& cloudResponse,
                                const IdentitiesData& identities,
                                const std::string& token ) const {
        return sshTunnel.getTunnelsData( cloudResponse, identities, token );
    }
};

TEST_F( SshTunnelTests, noTunnels ) {  // 1
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel ).Times( 0 );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel ).Times( 0 );
    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    waitTimer( std::chrono::seconds( 20 ) );
}

TEST_F( SshTunnelTests, startCameraTunnelsFromDatabase ) {  // 2.1
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    auto token = provision.encryptSensitiveData( "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9" );
    JsonArray dbTunnelsCamera;
    dbTunnelsCamera.push_back( JSON( "tunnelLink"
                                     << "192.168.0.0"
                                     << "tunnelPort" << 8000 << "remotePort" << 1 << "localPort" << 10 << "identity"
                                     << "identityCamera" ) );
    auto cameraData =
        JSON( "_id"
              << "cloud.tunnel_camera_coordinator"
              << "method"
              << "cloud.tunnel_camera_coordinator"
              << "token" << token << "tokenExpirationDate" << 2147483647 << "tunnels" << dbTunnelsCamera );

    JsonArray dbTunnelsSupport;
    dbTunnelsSupport.push_back( JSON( "tunnelLink"
                                      << "192.168.0.2"
                                      << "tunnelPort" << 8002 << "remotePort" << 1 << "localPort" << 20 << "identity"
                                      << "identitySupport" ) );
    auto supportData =
        JSON( "_id"
              << "cloud.tunnel_support_coordinator"
              << "method"
              << "cloud.tunnel_support_coordinator"
              << "token" << token << "tokenExpirationDate" << 2147483647 << "tunnels" << dbTunnelsSupport );

    auto& dbClient = getHostService().getDBClient();
    dbClient.insert( TempNetworkCloudSshTunnelsDBName, cameraData );
    dbClient.insert( TempNetworkCloudSshTunnelsDBName, supportData );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 23 ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 1 );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    waitTimer( std::chrono::seconds( 20 ) );
}

TEST_F( SshTunnelTests, startSupportTunnelsFromDatabase ) {  // 2.2
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    auto token = provision.encryptSensitiveData( "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9" );
    JsonArray dbTunnelsCamera;
    dbTunnelsCamera.push_back( JSON( "tunnelLink"
                                     << "192.168.0.0"
                                     << "tunnelPort" << 8000 << "remotePort" << 1 << "localPort" << 10 << "identity"
                                     << "identityCamera" ) );
    auto cameraData =
        JSON( "_id"
              << "cloud.tunnel_camera_coordinator"
              << "method"
              << "cloud.tunnel_camera_coordinator"
              << "token" << token << "tokenExpirationDate" << 2147483647 << "tunnels" << dbTunnelsCamera );

    JsonArray dbTunnelsSupport;
    dbTunnelsSupport.push_back( JSON( "tunnelLink"
                                      << "192.168.0.2"
                                      << "tunnelPort" << 8002 << "remotePort" << 1 << "localPort" << 22 << "identity"
                                      << "identitySupport" ) );
    auto supportData =
        JSON( "_id"
              << "cloud.tunnel_support_coordinator"
              << "method"
              << "cloud.tunnel_support_coordinator"
              << "token" << token << "tokenExpirationDate" << 2147483647 << "tunnels" << dbTunnelsSupport );

    auto& dbClient = getHostService().getDBClient();
    dbClient.insert( TempNetworkCloudSshTunnelsDBName, cameraData );
    dbClient.insert( TempNetworkCloudSshTunnelsDBName, supportData );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 22;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.2";
    tunnelData.serverPort = 8002;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 23 ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 1 );

    auto& fakeService = getHostService();
    SshSupportTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    int endTime = 2000000000;
    sshTunnel.createSupportTunnel( endTime );
    for( size_t i = 0; i < 10; ++i )
        waitTimer( std::chrono::seconds( 20 ) );
}

TEST_F( SshTunnelTests, startDisabledSupportTunnelsFromDatabase ) {  // 2.3
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    auto token = provision.encryptSensitiveData( "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9" );
    JsonArray dbTunnels;
    dbTunnels.push_back( JSON( "tunnelLink"
                               << "192.168.0.0"
                               << "tunnelPort" << 8000 << "remotePort" << 1 << "localPort" << 0 << "identity"
                               << "identityCamera" ) );
    auto Data = JSON( "_id"
                      << "cloud.tunnel_support_coordinator"
                      << "method"
                      << "cloud.tunnel_support_coordinator"
                      << "token" << token << "tokenExpirationDate" << 2147483647 << "tunnels" << dbTunnels );

    auto& dbClient = getHostService().getDBClient();
    dbClient.insert( TempNetworkCloudSshTunnelsDBName, Data );

    auto query = JSON( "supportSettings" << JSON( "enabled" << false << "endTime" << 2000000000 ) );
    dbClient.insert( NetworkCloudSshTunnelsSettingsDBName, query );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).Times( 0 );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 0 );

    auto& fakeService = getHostService();
    SshSupportTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    for( size_t i = 0; i < 10; ++i )
        waitTimer( std::chrono::seconds( 20 ) );
}

TEST_F( SshTunnelTests, sendTokenRequest ) {  // 3
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel ).Times( 0 );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel ).Times( 0 );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    auto message = getFakeBus().getMessage();
    checkTokenRequest( message,
                       JSON_ARRAY_( JSON( "identity"
                                          << "identity1" ) ),
                       JSON_EMPTY_ARR,
                       __LINE__ );
}

TEST_F( SshTunnelTests, invalidResponseFromCloud ) {  // 4
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel ).Times( 0 );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel ).Times( 0 );
    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    auto sendInvalidResponse = [this, &fakeService, &sshTunnel]( const Json& response, int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        auto message = getFakeBus().getMessage();
        checkTokenRequest( message,
                           JSON_ARRAY_( JSON( "identity"
                                              << "identity1" ) ),
                           JSON_EMPTY_ARR,
                           codeLine );
        auto sender = json_get_str( message.data, "sender" );
        auto uuid = json_get_str( message.data, "uuid" );
        fakeService.sendAsyncResponse( sender, response, uuid );
        waitTimer( std::chrono::seconds( 10 ) );
        static int isInternetAvailable = 0;
        std::chrono::seconds tryConnectionTimeout;
        if( isInternetAvailable ) {
            tryConnectionTimeout = InternetAvailable::TryConnectionTimeout;
        }
        else {
            tryConnectionTimeout = InternetNotAvailable::TryConnectionTimeout;
        }
        EXPECT_EQ( getTimeout( sshTunnel ), tryConnectionTimeout ) << codeLineStr;
        waitTimer( tryConnectionTimeout );
        isInternetAvailable = ( isInternetAvailable + 1 ) & 1;
        sshTunnel.setInternetAvailability( isInternetAvailable );
    };

    // empty response from cloud
    sendInvalidResponse( JSON_EMPTY_OBJ, __LINE__ );

    // error response from cloud
    sendInvalidResponse( JSON( "error" << JSON( "message"
                                                << "error" ) ),
                         __LINE__ );

    // empty token field
    sendInvalidResponse( JSON( "result" << JSON( "token" << JSON_EMPTY_OBJ ) ), __LINE__ );

    // empty string field
    sendInvalidResponse( JSON( "result" << JSON( "token" << JSON( "expires_ts" << 1606402220 ) ) ), __LINE__ );

    // empty expires_ts field
    sendInvalidResponse( JSON( "result" << JSON( "token" << JSON( "string"
                                                                  << "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9" ) ) ),
                         __LINE__ );

    auto token = JSON( "token" << JSON( "string"
                                        << "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"
                                        << "expires_ts" << 1606402220 ) );

    // empty endpoints field
    sendInvalidResponse( JSON( "result" << JSON( token << "endpoints" << JSON_EMPTY_OBJ ) ), __LINE__ );

    // empty ssh field
    sendInvalidResponse( JSON( "result" << JSON( token << "endpoints" << JSON( "ssh" << JSON_EMPTY_ARR ) ) ),
                         __LINE__ );

    // invalid endpoint
    sendInvalidResponse(
        JSON( "result" << JSON( token << "endpoints" << JSON( "ssh" << JSON_ARRAY_( "192.168.0.0" ) ) ) ), __LINE__ );
}

TEST_F( SshTunnelTests, startSingleTunnel ) {  // 5.1
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 23 ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ( dbData[ 0 ], getDbData( { { "identity1", tunnelData } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
}

TEST_F( SshTunnelTests, startTripleTunnels ) {  // 5.2
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    Ssh::TunnelManager::Data tunnelData3;
    tunnelData3.ownPort = 12;
    tunnelData3.remotePort = 3;
    tunnelData3.serverAddress = "192.168.0.2";
    tunnelData3.serverPort = 8002;
    tunnelData3.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData3.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData3, _ ) ).WillOnce( Return( 25 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 25 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    sshTunnel.createCameraTunnel( "identity3", 12 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ),
                                   JSON( "identity"
                                         << "identity3" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001", "192.168.0.2:8002" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );

    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ(
        dbData[ 0 ],
        getDbData( { { "identity1", tunnelData1 }, { "identity2", tunnelData2 }, { "identity3", tunnelData3 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 3 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
    EXPECT_EQ( json_get_str( dbData[ 2 ], "identity" ), "identity3" );
    EXPECT_EQ( json_get_int( dbData[ 2 ], "processId" ), 25 );
}

TEST_F( SshTunnelTests, startAndUpdateTunnels ) {  // 5.3
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 2 ).WillRepeatedly( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );
    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
}

TEST_F( SshTunnelTests, removeTunnelInCreatedState ) {  // 6.1
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    Ssh::TunnelManager::Data tunnelData3;
    tunnelData3.ownPort = 12;
    tunnelData3.remotePort = 3;
    tunnelData3.serverAddress = "192.168.0.2";
    tunnelData3.serverPort = 8002;
    tunnelData3.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData3.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData3, _ ) ).WillOnce( Return( 25 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );  // global teardown
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );  // removeTunnel() line
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 25 ) ).WillOnce( Return() );  // global teardown

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    sshTunnel.createCameraTunnel( "identity3", 12 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ),
                                   JSON( "identity"
                                         << "identity3" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001", "192.168.0.2:8002" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.removeTunnel( "identity2" );

    // not expecting any token requests, since identity2 is cached

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ( dbData[ 0 ], getDbData( { { "identity1", tunnelData1 }, { "identity3", tunnelData3 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity3" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 25 );
}

TEST_F( SshTunnelTests, removeLastTunnel ) {  // 6.2
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.removeTunnel( "identity1" );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 0 );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 0 );
}

TEST_F( SshTunnelTests, removeTunnelInRequestState ) {  // 6.3
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    sshTunnel.createCameraTunnel( "identity3", 12 );
    waitTimer( std::chrono::seconds( 20 ) );

    sshTunnel.removeTunnel( "identity3" );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ),
                                   JSON( "identity"
                                         << "identity3" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001", "192.168.0.2:8002" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity2", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ( dbData[ 0 ], getDbData( { { "identity1", tunnelData1 }, { "identity2", tunnelData2 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
}

TEST_F( SshTunnelTests, removeAndCreateTunnelInRequestState1 ) {  // 6.4
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    waitTimer( std::chrono::seconds( 20 ) );

    sshTunnel.removeTunnel( "identity2" );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity2", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ( dbData[ 0 ], getDbData( { { "identity1", tunnelData1 }, { "identity2", tunnelData2 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
}

TEST_F( SshTunnelTests, removeAndCreateTunnelInRequestState2 ) {  // 6.5
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).Times( 2 ).WillRepeatedly( Return( 23 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 2 ).WillRepeatedly( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    sshTunnel.removeTunnel( "identity2" );

    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ( dbData[ 0 ], getDbData( { { "identity1", tunnelData1 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
}

TEST_F( SshTunnelTests, removeTunnelInStopState1 ) {  // 6.6
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).Times( 0 );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).Times( 0 );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 0 );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).Times( 0 );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    EXPECT_NO_THROW( getTunnels( sshTunnel ).find( "identity1" )->second.state = SshTunnel::TunnelState::Running );
    EXPECT_NO_THROW( getTunnels( sshTunnel ).find( "identity2" )->second.state = SshTunnel::TunnelState::Running );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.removeTunnel( "identity2" );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ( dbData[ 0 ], getDbData( { { "identity1", tunnelData1 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 0 );
}

TEST_F( SshTunnelTests, removeTunnelInStopState2 ) {  // 6.7
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).Times( 0 );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).Times( 0 );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 0 );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).Times( 0 );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    EXPECT_NO_THROW( getTunnels( sshTunnel ).find( "identity1" )->second.state = SshTunnel::TunnelState::Running );
    EXPECT_NO_THROW( getTunnels( sshTunnel ).find( "identity2" )->second.state = SshTunnel::TunnelState::Running );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.removeTunnel( "identity2" );

    waitTimer( std::chrono::seconds( 1 ) );

    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ( dbData[ 0 ], getDbData( { { "identity1", tunnelData1 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 0 );
}

TEST_F( SshTunnelTests, removeTunnelInStopStateTimeout ) {  // 6.8
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).Times( 0 );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).Times( 0 );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 0 );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).Times( 0 );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    EXPECT_NO_THROW( getTunnels( sshTunnel ).find( "identity1" )->second.state = SshTunnel::TunnelState::Running );
    EXPECT_NO_THROW( getTunnels( sshTunnel ).find( "identity2" )->second.state = SshTunnel::TunnelState::Running );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.removeTunnel( "identity2" );

    waitTimer( std::chrono::seconds( 10 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ( dbData[ 0 ], getDbData( { { "identity1", tunnelData1 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 0 );
}

TEST_F( SshTunnelTests, removeAndCreateTunnelInCreatedState ) {  // 6.9
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    // createCameraTunnel() -> removeTunnel() -> createCameraTunnel() -> Global teardown
    Ssh::TunnelManager::Data tunnelData1(
        "192.168.0.0", 8000, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9", {}, 1, 10, getKeepAliveTimeout() );

    // createCameraTunnel() -> Global teardown
    Ssh::TunnelManager::Data tunnelData2(
        "192.168.0.1", 8001, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9", {}, 2, 11, getKeepAliveTimeout() );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).Times( 2 ).WillRepeatedly( Return( 23 ) );
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 2 );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).Times( 1 );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );
    waitTimer( std::chrono::seconds( 1 ) );

    // All tunnels started
    checkDbData( getDbData( { { "identity1", tunnelData1 }, { "identity2", tunnelData2 } } ), 2, __LINE__ );

    sshTunnel.removeTunnel( "identity1" );
    waitTimer( std::chrono::seconds( 1 ) );

    // 1st tunnel is cached, 2nd tunnel is connected
    checkDbData( getDbData( { { "identity2", tunnelData2 } } ), 1, __LINE__ );

    // no token requests expected since the tunnel is cached
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 1 ) );

    // Both tunnels are connected again
    checkDbData( getDbData( { { "identity1", tunnelData1 }, { "identity2", tunnelData2 } } ), 2, __LINE__ );
}

TEST_F( SshTunnelTests, tunnelsUpdatedDuringStopping ) {  // 7
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 2 ).WillRepeatedly( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 25 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.createCameraTunnel( "identity3", 12 );

    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    Ssh::TunnelManager::Data tunnelData3;
    tunnelData3.ownPort = 12;
    tunnelData3.remotePort = 3;
    tunnelData3.serverAddress = "192.168.0.2";
    tunnelData3.serverPort = 8001;
    tunnelData3.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData3.keepAliveTimeoutSec = getKeepAliveTimeout();

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData3, _ ) ).WillOnce( Return( 25 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ),
                                   JSON( "identity"
                                         << "identity3" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001", "192.168.0.2:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );
    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 3 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
    EXPECT_EQ( json_get_str( dbData[ 2 ], "identity" ), "identity3" );
    EXPECT_EQ( json_get_int( dbData[ 2 ], "processId" ), 25 );
}

TEST_F( SshTunnelTests, tunnelStoppingTimeout ) {  // 8
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 2 ).WillRepeatedly( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    waitTimer( std::chrono::seconds( 10 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
}

TEST_F( SshTunnelTests, lessSshConnectionsThanTunnels ) {  // 9
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.0";
    tunnelData2.serverPort = 8000;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    Ssh::TunnelManager::Data tunnelData3;
    tunnelData3.ownPort = 12;
    tunnelData3.remotePort = 3;
    tunnelData3.serverAddress = "192.168.0.0";
    tunnelData3.serverPort = 8000;
    tunnelData3.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData3.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData3, _ ) ).WillOnce( Return( 25 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 25 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    sshTunnel.createCameraTunnel( "identity2", 11 );
    sshTunnel.createCameraTunnel( "identity3", 12 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ),
                                   JSON( "identity"
                                         << "identity3" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    const auto methodQuery = JSON( "method" << JSON( "$exists" << true ) );
    auto dbData = dbClient.findAll( TempNetworkCloudSshTunnelsDBName, methodQuery, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    dbData[ 0 ][ "token" ] = provision.decryptSensitiveData( dbData[ 0 ][ "token" ] );
    dbData[ 0 ].erase( "_id" );
    dbData[ 0 ].erase( "status" );
    EXPECT_EQ(
        dbData[ 0 ],
        getDbData( { { "identity1", tunnelData1 }, { "identity2", tunnelData2 }, { "identity3", tunnelData3 } } ) );

    dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 3 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
    EXPECT_EQ( json_get_str( dbData[ 2 ], "identity" ), "identity3" );
    EXPECT_EQ( json_get_int( dbData[ 2 ], "processId" ), 25 );
}

TEST_F( SshTunnelTests, updatedDuringTokenRequest ) {  // 10
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 23 ) );

    tunnelData.ownPort = 11;
    tunnelData.remotePort = 2;
    tunnelData.serverAddress = "192.168.0.1";
    tunnelData.serverPort = 8001;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 24 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
}

TEST_F( SshTunnelTests, tokenExpired ) {  // 11
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).Times( 2 ).WillRepeatedly( Return( 23 ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 2 );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    {
        auto message = getFakeBus().getMessage();
        checkTokenRequest( message,
                           JSON_ARRAY_( JSON( "identity"
                                              << "identity1" ) ),
                           JSON_EMPTY_ARR,
                           __LINE__ );
        auto sender = json_get_str( message.data, "sender" );
        auto uuid = json_get_str( message.data, "uuid" );
        auto response = getTokenResponse( { "192.168.0.0:8000" }, 0 );
        fakeService.sendAsyncResponse( sender, response, uuid );
    }

    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData } );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
}

TEST_F( SshTunnelTests, requestTimeout ) {  // 12
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 23 ) );

    tunnelData.ownPort = 11;
    tunnelData.remotePort = 2;
    tunnelData.serverAddress = "192.168.0.1";
    tunnelData.serverPort = 8001;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 24 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    auto message = getFakeBus().getMessage();
    checkTokenRequest( message,
                       JSON_ARRAY_( JSON( "identity"
                                          << "identity1" ) ),
                       JSON_EMPTY_ARR,
                       __LINE__ );

    waitTimer( std::chrono::seconds( 10 ) );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    waitTimer( std::chrono::seconds( 60 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
}

TEST_F( SshTunnelTests, updateTunnelsDuringMonitoring ) {  // 13
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).Times( 2 ).WillRepeatedly( Return( 23 ) );

    tunnelData.ownPort = 11;
    tunnelData.remotePort = 2;
    tunnelData.serverAddress = "192.168.0.1";
    tunnelData.serverPort = 8001;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 24 ) );

    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).Times( 2 ).WillRepeatedly( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData } );

    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    waitTimer( std::chrono::seconds( 1 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );

    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData } );

    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
}

TEST_F( SshTunnelTests, rejectedTunnel ) {  // 14.1
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();

    using Status = Process::Forker::Status;

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) )
        .WillOnce( DoAll( InvokeArgument< 1 >( 23, Status::Ok, 5, std::string() ), Return( 23 ) ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );
    waitTimer( std::chrono::seconds( 1 ) );
    waitTimer( std::chrono::seconds( 70 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 23 ) );

    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData } );
    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
}

TEST_F( SshTunnelTests, closedTunnelByUnknownReason ) {  // 14.2
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData;
    tunnelData.ownPort = 10;
    tunnelData.remotePort = 1;
    tunnelData.serverAddress = "192.168.0.0";
    tunnelData.serverPort = 8000;
    tunnelData.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData.keepAliveTimeoutSec = getKeepAliveTimeout();

    using Status = Process::Forker::Status;

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) )
        .WillOnce( DoAll( InvokeArgument< 1 >( 23, Status::TermSignal, 56, std::string() ), Return( 23 ) ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );
    waitTimer( std::chrono::seconds( 1 ) );
    waitTimer( std::chrono::seconds( 60 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData, _ ) ).WillOnce( Return( 23 ) );

    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData } );
    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 1 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
}

TEST_F( SshTunnelTests, closedTunnel ) {  // 14.3
    TunnelManagerMock tunnelManagerMock;
    Services::UT::TaskLoopTestHelper::Counter counter;
    getFakeBus().setGroupFilter( { GroupsNames::UI_GROUP, "test_service_name" } );

    Ssh::TunnelManager::Data tunnelData1;
    tunnelData1.ownPort = 10;
    tunnelData1.remotePort = 1;
    tunnelData1.serverAddress = "192.168.0.0";
    tunnelData1.serverPort = 8000;
    tunnelData1.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData1.keepAliveTimeoutSec = getKeepAliveTimeout();

    using Status = Process::Forker::Status;

    std::string reason = "unknown reason";
    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) )
        .WillOnce( DoAll( InvokeArgument< 1 >( 23, Status::Ok, 255, ByRef( reason ) ), Return( 23 ) ) );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 23 ) ).WillOnce( Return() );
    EXPECT_CALL( tunnelManagerMock, killSshTunnel( 24 ) ).WillOnce( Return() );

    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    sshTunnel.initTunnels();
    sshTunnel.createCameraTunnel( "identity1", 10 );
    waitTimer( std::chrono::seconds( 20 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ) ),
                      { "192.168.0.0:8000" },
                      __LINE__ );

    waitTimer( std::chrono::seconds( 1 ) );
    waitTimer( std::chrono::seconds( 1 ) );

    sshTunnel.createCameraTunnel( "identity2", 11 );

    waitTimer( std::chrono::seconds( 60 ) );

    sendCloudRespose( fakeService,
                      JSON_ARRAY_( JSON( "identity"
                                         << "identity1" ),
                                   JSON( "identity"
                                         << "identity2" ) ),
                      { "192.168.0.0:8000", "192.168.0.1:8001" },
                      __LINE__ );

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData1, _ ) ).WillOnce( Return( 23 ) );

    Ssh::TunnelManager::Data tunnelData2;
    tunnelData2.ownPort = 11;
    tunnelData2.remotePort = 2;
    tunnelData2.serverAddress = "192.168.0.1";
    tunnelData2.serverPort = 8001;
    tunnelData2.serverUser = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    tunnelData2.keepAliveTimeoutSec = getKeepAliveTimeout();

    EXPECT_CALL( tunnelManagerMock, createSshTunnel( tunnelData2, _ ) ).WillOnce( Return( 24 ) );

    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity1", tunnelData1 } );
    waitTimer( std::chrono::seconds( 1 ) );
    getState( sshTunnel ).onTunnelWasClosed( { "identity2", tunnelData2 } );
    waitTimer( std::chrono::seconds( 1 ) );

    auto& dbClient = fakeService.getDBClient();
    auto dbData = dbClient.findAll( TempTunnelProcessIdsDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( dbData.size(), 2 );
    EXPECT_EQ( json_get_str( dbData[ 0 ], "identity" ), "identity1" );
    EXPECT_EQ( json_get_int( dbData[ 0 ], "processId" ), 23 );
    EXPECT_EQ( json_get_str( dbData[ 1 ], "identity" ), "identity2" );
    EXPECT_EQ( json_get_int( dbData[ 1 ], "processId" ), 24 );
}

TEST_F( SshTunnelTests, randomBalancing ) {  // 15
    TunnelManagerMock tunnelManagerMock;
    auto& fakeService = getHostService();
    SshCameraTunnel sshTunnel( fakeService, tunnelManagerMock );
    auto response = getTokenResponse( { "192.168.0.0:8000", "192.168.0.1:8001" } );
    auto tunnels = getTunnelsData(
        sshTunnel,
        response,
        { { "identity0", 0 }, { "identity1", 1 }, { "identity2", 2 }, { "identity3", 3 }, { "identity4", 4 } },
        "token" );

    std::set< std::string > expectedLinks = { "192.168.0.0", "192.168.0.1" };
    std::set< int > expectedPorts = { 8000, 8001 };

    auto data = tunnels[ 0 ];
    EXPECT_EQ( data.identity, "identity0" );
    EXPECT_EQ( data.data.serverUser, "token" );
    EXPECT_EQ( data.data.ownPort, 0 );
    EXPECT_EQ( data.data.remotePort, 1 );
    EXPECT_TRUE( expectedLinks.find( data.data.serverAddress ) != expectedLinks.end() );
    EXPECT_TRUE( expectedPorts.find( data.data.serverPort ) != expectedPorts.end() );

    data = tunnels[ 1 ];
    EXPECT_EQ( data.identity, "identity1" );
    EXPECT_EQ( data.data.serverUser, "token" );
    EXPECT_EQ( data.data.ownPort, 1 );
    EXPECT_EQ( data.data.remotePort, 2 );
    EXPECT_TRUE( expectedLinks.find( data.data.serverAddress ) != expectedLinks.end() );
    EXPECT_TRUE( expectedPorts.find( data.data.serverPort ) != expectedPorts.end() );

    data = tunnels[ 2 ];
    EXPECT_EQ( data.identity, "identity2" );
    EXPECT_EQ( data.data.serverUser, "token" );
    EXPECT_EQ( data.data.ownPort, 2 );
    EXPECT_EQ( data.data.remotePort, 3 );
    EXPECT_TRUE( expectedLinks.find( data.data.serverAddress ) != expectedLinks.end() );
    EXPECT_TRUE( expectedPorts.find( data.data.serverPort ) != expectedPorts.end() );

    data = tunnels[ 3 ];
    EXPECT_EQ( data.identity, "identity3" );
    EXPECT_EQ( data.data.serverUser, "token" );
    EXPECT_EQ( data.data.ownPort, 3 );
    EXPECT_EQ( data.data.remotePort, 4 );
    EXPECT_TRUE( expectedLinks.find( data.data.serverAddress ) != expectedLinks.end() );
    EXPECT_TRUE( expectedPorts.find( data.data.serverPort ) != expectedPorts.end() );

    data = tunnels[ 4 ];
    EXPECT_EQ( data.identity, "identity4" );
    EXPECT_EQ( data.data.serverUser, "token" );
    EXPECT_EQ( data.data.ownPort, 4 );
    EXPECT_EQ( data.data.remotePort, 5 );
    EXPECT_TRUE( expectedLinks.find( data.data.serverAddress ) != expectedLinks.end() );
    EXPECT_TRUE( expectedPorts.find( data.data.serverPort ) != expectedPorts.end() );
}

}  // namespace UT
}  // namespace SshTunnel
}  // namespace Network
}  // namespace HubPlatform
