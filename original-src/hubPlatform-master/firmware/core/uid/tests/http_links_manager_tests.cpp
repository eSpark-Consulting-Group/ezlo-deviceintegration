#include "uid/extensions/http_links/http_links_manager.h"
#include "uid/extensions/http_links/resources.h"
#include "common_utils/cpp14.h"
#include "bus/fake_bus.h"
#include "service_interfaces/service.h"
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "database_client/fake_db/fake_db_client.h"
#include "database_client/dbqueryparser.h"
#include "database_client/dbtools.h"
#include <gtest/gtest.h>
#include <utility>

namespace HubPlatform {
namespace HttpLinks {
namespace UT {

/*

TESTPLAN

TODO 1 HttpLinksManager links
DONE    1.1 Create an http link object
DONE        1.1.1 Create an anonymous http link object
DONE        1.1.2 Create a named http link object
DONE    1.2 Delete an http link object
DONE    1.3 Get http link info
TODO    1.4 Get http link info raw json data
DONE    1.5 makeInsecureLink
DONE    1.6 makeSecureLink
DONE    1.7 Clear deleted links
DONE    1.8 Update current IP

DONE 2 HttpLinksManager Connections
DONE    2.1 Create a connection
DONE    2.2 Get connection info
DONE    2.3 Close a connection
DONE    2.4 mark connection as closed
DONE    2.5 Clear closed connections
TODO    2.6 Move portions of data to a buffer

 */

namespace Resources {
const char* const defaultHostIp = "127.1.1.1";
const int securePort = 17443;
const int insecurePort = 17080;
}  // namespace Resources

class EventHandlerFake final : public HttpLinks::HttpLinksEventHandler {
public:
    void onHttpDataReceived( const EventBase& event ) override {
        LOGGER_TRACE( "" );
    }
    void onHttpConnectionClosed( const EventBase& event ) override {
        LOGGER_TRACE( "" );
    }
    void onHttpDataSend( const EventBase& event ) override {
        LOGGER_TRACE( "" );
    }
    void onHttpLinkUpdated( const EventBase& event ) override {
        LOGGER_TRACE( "" );
        m_linkUpdated++;
    }
    int linkUpdatedCount() const {
        return m_linkUpdated;
    }

    void clear() {
        m_linkUpdated = 0;
    }

private:
    std::atomic_int m_linkUpdated{ 0 };
};

class FakeService final : public Services::Service {
public:
    FakeService()
    : Services::Service( "",
                         std::make_unique< Services::BusWrapper >(
                             Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ),
                         std::make_unique< HubPlatform::UT::FakeDbClient >() ) {}

    void setUp() override {
        Services::Service::setUp();
    }

    void tearDown() override {
        Services::Service::tearDown();
    }

    BaseBus* getBus() {
        return Services::Service::getBus();
    }
};

class HttpLinksManagerTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;
    HttpLinks::HttpLinksManager& manager() {
        return *m_manager;
    }
    HttpLinks::HttpLinksEventHandler& eventHandler() {
        return m_eventHandler;
    }

    std::string makeHandlerPath( const std::string& handlerName = "event_handler" );

    void createLink( std::string& linkId,
                     const std::string& eventHandler,
                     Json userData,
                     int timeout,
                     bool answer,
                     const char* file,
                     int line,
                     bool noThrow = true );

protected:
    using HttpLinksManagerUPtr = std::unique_ptr< HttpLinks::HttpLinksManager >;
    FakeService m_service;
    EventHandlerFake m_eventHandler;
    HttpLinksManagerUPtr m_manager;
    std::string m_pluginId{ "uid_tests" };
};

void HttpLinksManagerTest::SetUp() {
    Test::SetUp();
    m_manager.reset( new HttpLinks::HttpLinksManager( m_service, m_eventHandler ) );
    m_manager->setHostOptions( Resources::defaultHostIp, Resources::insecurePort, Resources::securePort );
}

void HttpLinksManagerTest::TearDown() {
    Test::TearDown();
    m_service.getDBClient().clear( DbNames::HttpLinks );
    m_service.getDBClient().clear( DbNames::TempHttpLinksConnections );
    m_eventHandler.clear();
}

std::string HttpLinksManagerTest::makeHandlerPath( const std::string& handlerName ) {
    return "HUB:" + m_pluginId + "/scripts/handlers/" + handlerName + ".lua";
}

void HttpLinksManagerTest::createLink( std::string& linkId,
                                       const std::string& eventHandler,
                                       Json userData,
                                       int timeout,
                                       bool answer,
                                       const char* file,
                                       int line,
                                       bool noThrow ) {
    std::string handlerScript = makeHandlerPath( eventHandler );
    HttpLink link( m_pluginId, linkId, handlerScript, std::move( userData ), timeout, answer );

    if( noThrow ) {
        ASSERT_NO_THROW( linkId = manager().createHttpLink( link ) ) << "should not throw" << file << ":" << line;
        ASSERT_FALSE( linkId.empty() ) << "Should be not empty";
    }
    else {
        ASSERT_ANY_THROW( linkId = manager().createHttpLink( link ) )
            << "should throw something" << file << ":" << line;
    }
}

TEST_F( HttpLinksManagerTest, createHttpLink ) /* 1.1.1 */ {
    std::string linkId;
    createLink( linkId,
                "handler",
                JSON( "test"
                      << "createHttpLink" ),
                500,
                false,
                __FILE__,
                __LINE__,
                true );
}

TEST_F( HttpLinksManagerTest, createHttpLinkNamed ) /* 1.1.2 */ {
    std::string linkName = "named_link";
    Json userData = Json{ { "test", "createHttpLinkNamed" } };
    const int timeout = 30000;
    const bool answer = false;

    bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, !noThrow );
}

TEST_F( HttpLinksManagerTest, deleteHttpLinkNamed ) /* 1.2 */ {
    std::string linkName = "named_link";
    Json userData = Json{ { "test", "deleteHttpLinkNamed" } };
    const int timeout = 30000;
    const bool answer = false;

    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );
    Json rawLink = manager().getLinkInfoRaw( linkName );
    EXPECT_FALSE( rawLink.empty() );
    EXPECT_EQ( json_get_int( rawLink, "deleted", -1 ), 0 );

    ASSERT_TRUE( manager().deleteHttpLink( m_pluginId, linkName ) );

    // check if it was marked for delete
    rawLink = manager().getLinkInfoRaw( linkName );
    EXPECT_FALSE( rawLink.empty() ) << "record should be present";
    EXPECT_GT( json_get_int( rawLink, "deleted", -1 ), 0 ) << "should be marked for deletion";
}

TEST_F( HttpLinksManagerTest, getLinkInfo ) /* 1.3 */ {
    std::string linkName = "named_link";
    Json userData = Json{ { "test", "getLinkInfo" } };
    const int timeout = 30000;
    const bool answer = false;

    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );
    Json linkInfo = manager().getLinkInfo( m_pluginId, linkName );
    LOGGER_TRACE( cc::j( linkInfo ) );
    EXPECT_FALSE( linkInfo.empty() );

    ASSERT_TRUE( manager().deleteHttpLink( m_pluginId, linkName ) );

    // check if it was marked for delete
    linkInfo = manager().getLinkInfoRaw( linkName );
    LOGGER_TRACE( cc::j( linkInfo ) );
    EXPECT_FALSE( linkInfo.empty() ) << "record should be present";

    // should be deleted
    linkInfo = manager().getLinkInfo( m_pluginId, linkName );
    LOGGER_TRACE( cc::j( linkInfo ) );
    EXPECT_TRUE( linkInfo.empty() ) << "deleted record should NOT be present";
}

TEST_F( HttpLinksManagerTest, makeInsecureLink ) /* 1.5 */ {
    std::string linkId = "111";
    std::string result = manager().makeInsecureLink( m_pluginId, linkId );
    LOGGER_TRACE( result );
    EXPECT_FALSE( result.empty() ) << result;
    EXPECT_FALSE( result.find( "http://" ) == std::string::npos );
    EXPECT_FALSE( result.find( m_pluginId ) == std::string::npos );
    EXPECT_FALSE( result.find( std::to_string( Resources::insecurePort ) ) == std::string::npos );
    EXPECT_FALSE( result.find( linkId ) == std::string::npos );
}

TEST_F( HttpLinksManagerTest, makeSecureLink ) /* 1.5 */ {
    std::string linkId = "111";
    std::string result = manager().makeSecureLink( m_pluginId, linkId );
    LOGGER_TRACE( result );
    EXPECT_FALSE( result.empty() ) << result;
    EXPECT_FALSE( result.find( "https://" ) == std::string::npos );
    EXPECT_FALSE( result.find( m_pluginId ) == std::string::npos );
    EXPECT_FALSE( result.find( std::to_string( Resources::securePort ) ) == std::string::npos );
    EXPECT_FALSE( result.find( linkId ) == std::string::npos );
}

TEST_F( HttpLinksManagerTest, clearDeletedLinks ) /* 1.7 */ {
    std::string linkName = std::string{}, linkName1 = std::string{};
    Json userData = Json{ { "test", "clearDeletedLinks" } };
    const int timeout = 30000;
    const bool answer = false;

    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );
    createLink( linkName1, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );

    // manually move link.deleted timestamp outside of safe interval
    const int safeIntervalSec = HttpLinks::Resources::Limits::HttpLinksLifetimeAfterDeletedSec;
    m_service.getDBClient().update(
        DbNames::HttpLinks,
        JSON( db::ID_FIELD_NAME << linkName ),
        JSON( "$set" << JSON( "deleted" << time( nullptr ) - ( safeIntervalSec + 100 ) ) ) );

    EXPECT_EQ( m_service.getDBClient().count( DbNames::HttpLinks, Json::object() ), 2 );

    manager().clearDeletedLinks();

    EXPECT_EQ( m_service.getDBClient().count( DbNames::HttpLinks, Json::object() ), 1 );
}

TEST_F( HttpLinksManagerTest, updateCurrentIP ) /* 1.8 */ {
    std::string linkName = std::string{};
    Json userData = Json{ { "test", "updateCurrentIP" } };
    const int timeout = 30000;
    const bool answer = false;

    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );

    const std::string newIpAddress = "192.168.43.46";
    Json updateIpEvent = R"({"interfaceId":"eth0","ipv4":"192.168.43.46","type":"ethernet"})"_json;

    manager().updateCurrentIp( updateIpEvent );

    Json rawLink = manager().getLinkInfoRaw( linkName );
    EXPECT_FALSE( rawLink.empty() );
    namespace Fields = HttpLinks::Resources::HttpLinkFields;
    EXPECT_FALSE( json_get_str( rawLink, Fields::url ).find( newIpAddress ) == std::string::npos );
    EXPECT_FALSE( json_get_str( rawLink, Fields::secureUrl ).find( newIpAddress ) == std::string::npos );

    EXPECT_EQ( m_eventHandler.linkUpdatedCount(), 1 );
}

TEST_F( HttpLinksManagerTest, createConnection ) /* 2.1. */ {
    std::string linkName;
    Json userData = Json{ { "test", "createConnection" } };
    const int timeout = 30000;
    const bool answer = true;
    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );

    ConnectionInfo connInfo( linkName, {}, {} );
    lws* wsi = nullptr;
    auto connId = manager().createConnection( linkName, connInfo, wsi );
    EXPECT_FALSE( connId.empty() ) << "connection id should be assigned";
}

TEST_F( HttpLinksManagerTest, getConnectionInfo ) /* 2.2 */ {
    std::string linkName;
    Json userData = Json{ { "test", "getConnectionInfo" } };
    const int timeout = 30000;
    const bool answer = true;
    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );

    ConnectionInfo connInfo( linkName, {}, {} );
    connInfo.clientIp( Resources::defaultHostIp );

    lws* wsi = nullptr;
    auto connId = manager().createConnection( linkName, connInfo, wsi );
    EXPECT_FALSE( connId.empty() ) << "connection id should be assigned";

    ASSERT_TRUE( manager().getConnectionInfo( connId, connInfo ) );
    LOGGER_TRACE( cc::j( connInfo.toJsonLua() ) );
    LOGGER_TRACE( cc::j( connInfo.toJsonDb() ) );
    EXPECT_EQ( connInfo.id(), connId );
    EXPECT_EQ( connInfo.linkId(), linkName );
    EXPECT_EQ( connInfo.clientIp(), Resources::defaultHostIp );
    EXPECT_EQ( connInfo.isClosed(), false );
    EXPECT_EQ( connInfo.headers(), ConnectionInfo::Headers{} );
    EXPECT_EQ( connInfo.params(), ConnectionInfo::Params{} );

    EXPECT_EQ( json_get_str( connInfo.toJsonLua(), "status" ), "pending" );
}

TEST_F( HttpLinksManagerTest, closeConnection ) /* 2.3 */ {
    std::string linkName;
    Json userData = Json{ { "test", "getConnectionInfo" } };
    const int timeout = 30000;
    const bool answer = true;
    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );

    ConnectionInfo connInfo( linkName, {}, {} );
    connInfo.clientIp( Resources::defaultHostIp );

    lws* wsi = nullptr;
    auto connId = manager().createConnection( linkName, connInfo, wsi );
    EXPECT_FALSE( connId.empty() ) << "connection id should be assigned";

    ASSERT_TRUE( manager().getConnectionInfo( connId, connInfo ) );
    EXPECT_EQ( connInfo.id(), connId );
    EXPECT_EQ( connInfo.linkId(), linkName );
    EXPECT_EQ( connInfo.clientIp(), Resources::defaultHostIp );
    EXPECT_EQ( connInfo.isClosed(), false );
    EXPECT_EQ( connInfo.headers(), ConnectionInfo::Headers{} );
    EXPECT_EQ( connInfo.params(), ConnectionInfo::Params{} );

    EXPECT_FALSE( manager().connectionForceClosed( connId ) );
    ASSERT_TRUE( manager().closeConnection( m_pluginId, connId ) );
    EXPECT_TRUE( manager().connectionForceClosed( connId ) );
}

TEST_F( HttpLinksManagerTest, markConnectionAsClosed ) /* 2.4 */ {
    std::string linkName;
    Json userData = Json{ { "test", "markConnectionAsClosed" } };
    const int timeout = 30000;
    const bool answer = true;
    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );

    ConnectionInfo connInfo( linkName, {}, {} );
    connInfo.clientIp( Resources::defaultHostIp );

    lws* wsi = nullptr;
    auto connId = manager().createConnection( linkName, connInfo, wsi );
    EXPECT_FALSE( connId.empty() ) << "connection id should be assigned";

    EXPECT_EQ( json_get_str( connInfo.toJsonLua(), "status" ), "pending" );

    EXPECT_TRUE( manager().markConnectionAsClosed( connId ) );
    ASSERT_TRUE( manager().getConnectionInfo( connId, connInfo ) );
    EXPECT_EQ( json_get_str( connInfo.toJsonLua(), "status" ), "closed" );
}

TEST_F( HttpLinksManagerTest, clearClosedConnections ) /* 2.5 */ {
    std::string linkName;
    Json userData = Json{ { "test", "clearClosedConnections" } };
    const int timeout = 30000;
    const bool answer = true;
    const bool noThrow = true;
    createLink( linkName, "handler", userData, timeout, answer, __FILE__, __LINE__, noThrow );

    ConnectionInfo connInfo( linkName, {}, {} );
    connInfo.clientIp( Resources::defaultHostIp );

    lws* wsi = nullptr;
    std::string connId0, connId1;
    connId0 = manager().createConnection( linkName, connInfo, wsi );
    EXPECT_FALSE( connId0.empty() ) << "connection id should be assigned";

    connId1 = manager().createConnection( linkName, connInfo, wsi );
    EXPECT_FALSE( connId1.empty() ) << "connection id should be assigned";

    EXPECT_TRUE( manager().markConnectionAsClosed( connId0 ) );
    ASSERT_TRUE( manager().getConnectionInfo( connId0, connInfo ) );
    EXPECT_EQ( json_get_str( connInfo.toJsonLua(), "status" ), "closed" );

    EXPECT_EQ( m_service.getDBClient().count( DbNames::TempHttpLinksConnections, JSON_EMPTY_OBJ ), 2 );

    // manually move connection.closed timestamp outside of safe interval
    using namespace HttpLinks::Resources;
    const int safeIntervalSec = Limits::HttpLinksConnectionsLifetimeAfterClosedSec + 100;
    m_service.getDBClient().update(
        DbNames::TempHttpLinksConnections,
        JSON( db::ID_FIELD_NAME << connId1 ),
        JSON( "$set" << JSON( ConnectionDbFields::created << time( nullptr ) << ConnectionDbFields::closed
                                                          << time( nullptr ) - ( safeIntervalSec + 100 ) ) ) );

    manager().clearClosedConnections();
    EXPECT_EQ( m_service.getDBClient().count( DbNames::TempHttpLinksConnections, JSON_EMPTY_OBJ ), 1 );

    Json rawConn = m_service.getDBClient().findOne(
        DbNames::TempHttpLinksConnections, JSON( db::ID_FIELD_NAME << connId1 ), nullptr );
    EXPECT_TRUE( rawConn.empty() );
}

}  // namespace UT
}  // namespace HttpLinks
}  // namespace HubPlatform
