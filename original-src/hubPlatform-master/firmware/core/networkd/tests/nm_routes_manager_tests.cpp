#include "networkd/routes/nm_openwrt_routes_manager.h"
#include "networkd/resources/nm_resources.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

#include <sstream>

/*

TESTPLAN

DONE    1. Openwrt Routes Manager Tests
DONE        1.1 Generic tests
DONE            1.1.1 Interface info fixture
DONE            1.1.2 Parse routes
DONE            1.1.3 Get default route device
DONE            1.1.4 Get non-existing default route device
DONE        1.2 Check and fix tests
DONE            1.2.1 Correct route
DONE            1.2.2 Add missing route
DONE            1.2.3 Add missing route for secondary interface
DONE            1.2.4 Replace outdated default route
DONE            1.2.5 Add missing metric route for wifi interface with internet
DONE            1.2.6 Add missing metric route for wifi interface without internet
DONE            1.2.7 Do not add missing metric route for inactive wifi interface
DONE            1.2.8 Replace manual routes with invalid metric
DONE            1.2.9 Add missing interface
DONE            1.2.10 Fix badly invalid routes

*/

namespace HubPlatform {
namespace UT {

class FakeOpenwrtRoutesActionHandler : public Network::OpenwrtRoutesManager::ActionHandlerInterface {
public:
    std::string readRoutesTable() override {
        return m_routesTable;
    }

    bool applyRouteAction( const std::string& action ) override {
        m_appliedActions.push_back( action );
        return true;
    }

    std::string m_routesTable;
    std::vector< std::string > m_appliedActions;
};

class NmOpenwrtRoutesManagerSuite : public ::testing::Test {
public:
    struct Interface {
        static std::string StatusUp() {
            return Network::Resources::InterfaceInfo::Statuses::Up;
        }
        static std::string StatusDown() {
            return Network::Resources::InterfaceInfo::Statuses::Down;
        }

        std::string id;
        int priority = 0;
        int metric = 0;
        std::string status = StatusDown();
        std::string gateway;
        bool internetAvailable = false;

        Json toJson() {
            using namespace Network::Resources::InterfaceInfo;
            auto json = JSON_EMPTY_OBJ;

            if( !id.empty() ) {
                json[ Fields::Id ] = id;
            }

            json[ Fields::Priority ] = priority;
            json[ Fields::Metric ] = metric;
            json[ Fields::Status ] = status;
            json[ Fields::InternetAvailable ] = internetAvailable;

            if( !gateway.empty() ) {
                json[ Fields::Ipv4 ][ Fields::Gateway ] = gateway;
            }

            return json;
        }
    };
};

TEST_F( NmOpenwrtRoutesManagerSuite, interfaceInfoFixture ) /* 1.1.1 */ {
    Interface info;
    info.id = "wlan0";
    info.priority = 1;
    info.metric = 200;
    info.status = Interface::StatusUp();
    info.gateway = "192.168.88.1";
    info.internetAvailable = true;

    auto json = info.toJson();

    using namespace Network::Resources::InterfaceInfo;
    EXPECT_EQ( json_get_str( json, Fields::Id ), "wlan0" );
    EXPECT_EQ( json_get_int( json, Fields::Priority ), 1 );
    EXPECT_EQ( json_get_int( json, Fields::Metric ), 200 );
    EXPECT_EQ( json_get_str( json, Fields::Status ), Interface::StatusUp() );
    EXPECT_EQ( json_get_str( json.at( Fields::Ipv4 ), Fields::Gateway ), "192.168.88.1" );
}

TEST_F( NmOpenwrtRoutesManagerSuite, parseRoutes ) /* 1.1.2 */ {
    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev wlan0
default via 192.168.88.1 dev wlan0 proto static src 192.168.88.14 metric 200
192.168.88.0/24 dev wlan0 proto static scope link metric 200)";
    // clang-format on

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    auto routes = manager.readRoutes();

    ASSERT_EQ( routes.size(), 3 );

    {
        auto& route = routes.at( 0 );
        EXPECT_EQ( route.routeString, "default via 192.168.88.1 dev wlan0" );
        EXPECT_EQ( route.isDefault, true );
        EXPECT_EQ( route.device, "wlan0" );
        EXPECT_EQ( route.gateway, "192.168.88.1" );
        EXPECT_EQ( route.metric, "" );
        EXPECT_EQ( route.proto, "" );
        EXPECT_EQ( route.source, "" );
    }

    {
        auto& route = routes.at( 1 );
        EXPECT_EQ( route.routeString, "default via 192.168.88.1 dev wlan0 proto static src 192.168.88.14 metric 200" );
        EXPECT_EQ( route.isDefault, true );
        EXPECT_EQ( route.device, "wlan0" );
        EXPECT_EQ( route.gateway, "192.168.88.1" );
        EXPECT_EQ( route.metric, "200" );
        EXPECT_EQ( route.proto, "static" );
        EXPECT_EQ( route.source, "192.168.88.14" );
    }

    {
        auto& route = routes.at( 2 );
        EXPECT_EQ( route.routeString, "192.168.88.0/24 dev wlan0 proto static scope link metric 200" );
        EXPECT_EQ( route.isDefault, false );
        EXPECT_EQ( route.device, "wlan0" );
        EXPECT_EQ( route.gateway, "" );
        EXPECT_EQ( route.metric, "200" );
        EXPECT_EQ( route.proto, "static" );
        EXPECT_EQ( route.source, "" );
    }
}

TEST_F( NmOpenwrtRoutesManagerSuite, getDefaultRouteDevice ) /* 1.1.3 */ {
    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev wlan0
default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
default via 192.168.88.1 dev wlan0 proto static src 192.168.88.14 metric 200
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.getDefaultRouteDevice(), "wlan0" );
}

TEST_F( NmOpenwrtRoutesManagerSuite, getNonExistingDefaultRouteDevice ) /* 1.1.4 */ {
    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"( default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
default via 192.168.88.1 dev wlan0 proto static src 192.168.88.14 metric 200
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.getDefaultRouteDevice(), "" )
        << "Manually set default route should not include 'metric' and 'proto' fields";
}

TEST_F( NmOpenwrtRoutesManagerSuite, checkAndFixCorrectRoute ) /* 1.2.1 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0
default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
default via 192.168.88.1 dev wlan0 proto static src 192.168.88.14 metric 200
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "eth0" );

    EXPECT_EQ( handlerRef.m_appliedActions.size(), 0 );
}

TEST_F( NmOpenwrtRoutesManagerSuite, addMissingRoute ) /* 1.2.2 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
default via 192.168.88.1 dev wlan0 proto static src 192.168.88.14 metric 200
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "eth0" );

    ASSERT_EQ( handlerRef.m_appliedActions.size(), 1 );
    EXPECT_EQ( handlerRef.m_appliedActions.at( 0 ), "ip route add default via 192.168.88.1 dev eth0" );
}

TEST_F( NmOpenwrtRoutesManagerSuite, addMissingRouteForSecondaryInterface ) /* 1.2.3 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = false;  // NOTE: no internet
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
default via 192.168.88.1 dev wlan0 proto static src 192.168.88.14 metric 200
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "wlan0" );

    ASSERT_EQ( handlerRef.m_appliedActions.size(), 1 );
    EXPECT_EQ( handlerRef.m_appliedActions.at( 0 ), "ip route add default via 192.168.88.1 dev wlan0" );
}

TEST_F( NmOpenwrtRoutesManagerSuite, replaceOutdatedDefaultRoute ) /* 1.2.4 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = false;  // NOTE: no internet
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0
default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
default via 192.168.88.1 dev wlan0 proto static src 192.168.88.14 metric 200
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "wlan0" );

    ASSERT_EQ( handlerRef.m_appliedActions.size(), 1 );
    EXPECT_EQ( handlerRef.m_appliedActions.at( 0 ), "ip route replace default via 192.168.88.1 dev wlan0" );
}

TEST_F( NmOpenwrtRoutesManagerSuite, addMissingMetricRouteForWifiInterfaceWithInternet ) /* 1.2.5 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0
default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "eth0" );

    ASSERT_EQ( handlerRef.m_appliedActions.size(), 1 );
    EXPECT_EQ( handlerRef.m_appliedActions.at( 0 ), "ip route add default via 192.168.88.1 dev wlan0 metric 200" );
}

TEST_F( NmOpenwrtRoutesManagerSuite, addMissingMetricRouteForWifiInterfaceWithoutInternet ) /* 1.2.6 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = false;  // NOTE: no internet
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0
default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "eth0" );

    ASSERT_EQ( handlerRef.m_appliedActions.size(), 1 );
    EXPECT_EQ( handlerRef.m_appliedActions.at( 0 ), "ip route add default via 192.168.88.1 dev wlan0 metric 200" );
}

TEST_F( NmOpenwrtRoutesManagerSuite, doNotAddMissingMetricRouteForInactiveWifiInterface ) /* 1.2.7 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "";               // NOTE: empty gateway
        info.internetAvailable = false;  // NOTE: no internet
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0
default via 192.168.88.1 dev eth0 proto static src 192.168.88.253 metric 100
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "eth0" );

    ASSERT_EQ( handlerRef.m_appliedActions.size(), 0 );
}

TEST_F( NmOpenwrtRoutesManagerSuite, replaceManualRoutesWithInvalidMetrics ) /* 1.2.8 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0
default via 192.168.88.1 dev eth0 metric 101
default via 192.168.88.1 dev wlan0 metric 201
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "eth0" );

    std::vector< std::string > expectedCommands = {
        "ip route del default via 192.168.88.1 dev eth0 metric 101",
        "ip route add default via 192.168.88.1 dev eth0 metric 100",
        "ip route del default via 192.168.88.1 dev wlan0 metric 201",
        "ip route add default via 192.168.88.1 dev wlan0 metric 200",
    };

    ASSERT_EQ( handlerRef.m_appliedActions.size(), expectedCommands.size() );

    std::sort( handlerRef.m_appliedActions.begin(), handlerRef.m_appliedActions.end() );
    std::sort( expectedCommands.begin(), expectedCommands.end() );

    EXPECT_EQ( handlerRef.m_appliedActions, expectedCommands );
}

TEST_F( NmOpenwrtRoutesManagerSuite, addMissingInterface ) /* 1.2.9 */ {
    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = false;  // NOTE: no internet
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0 metric 100
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "wlan0" );

    std::vector< std::string > expectedCommands = {
        "ip route add default via 192.168.88.1 dev wlan0",
        "ip route add default via 192.168.88.1 dev wlan0 metric 200",
    };

    ASSERT_EQ( handlerRef.m_appliedActions.size(), expectedCommands.size() );

    std::sort( handlerRef.m_appliedActions.begin(), handlerRef.m_appliedActions.end() );
    std::sort( expectedCommands.begin(), expectedCommands.end() );

    EXPECT_EQ( handlerRef.m_appliedActions, expectedCommands );
}

TEST_F( NmOpenwrtRoutesManagerSuite, fixBadlyInvalidRoutes ) /* 1.2.10 */ {
    // - eth0 route must be replace with wlan0 route
    // - eth0 metric route must be added
    // - wlan0 metric route must fixed

    std::vector< Json > interfacesList;
    {
        Interface info;
        info.id = "eth0";
        info.priority = 0;
        info.metric = 100;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = false;  // NOTE: no internet
        interfacesList.emplace_back( info.toJson() );
    }

    {
        Interface info;
        info.id = "wlan0";
        info.priority = 1;
        info.metric = 200;
        info.status = Interface::StatusUp();
        info.gateway = "192.168.88.1";
        info.internetAvailable = true;
        interfacesList.emplace_back( info.toJson() );
    }

    auto handler = std::make_unique< FakeOpenwrtRoutesActionHandler >();
    // clang-format off
    handler->m_routesTable = \
R"(default via 192.168.88.1 dev eth0
default via 192.168.88.1 dev wlan0 metric 201
192.168.88.0/24 dev wlan0 proto static scope link metric 200
192.168.88.0/24 dev eth0 proto static scope link metric 100)";
    // clang-format on

    auto& handlerRef = *handler;

    Network::OpenwrtRoutesManager manager( std::move( handler ) );
    EXPECT_EQ( manager.checkAndFixRoutes( interfacesList ), "wlan0" );

    std::vector< std::string > expectedCommands = {
        "ip route replace default via 192.168.88.1 dev wlan0",
        "ip route add default via 192.168.88.1 dev eth0 metric 100",
        "ip route del default via 192.168.88.1 dev wlan0 metric 201",
        "ip route add default via 192.168.88.1 dev wlan0 metric 200",
    };

    ASSERT_EQ( handlerRef.m_appliedActions.size(), expectedCommands.size() );

    std::sort( handlerRef.m_appliedActions.begin(), handlerRef.m_appliedActions.end() );
    std::sort( expectedCommands.begin(), expectedCommands.end() );

    EXPECT_EQ( handlerRef.m_appliedActions, expectedCommands );
}

}  // namespace UT
}  // namespace HubPlatform
