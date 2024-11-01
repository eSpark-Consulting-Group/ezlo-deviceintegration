#include "cameras/cameras_addon.h"
#include "networkd/resources/nm_public_resources.h"

#include "json_utils/json_utils.h"

#include <gmock/gmock.h>

using namespace ::testing;

namespace HubPlatform {
namespace Cameras {
namespace UT {

/*

    TEST PLAN
DONE    1. Ip address update
DONE        1.1. Ip address is updated with supported network interface types
DONE    2. Processing connected network interfaces
DONE        2.1. Ethernet interface is added to list of connected network interfaces
DONE        2.2. Wifi interface is added to list of connected network interfaces
DONE        2.3. Ethernet AND Wifi interfaces are added to list of connected network interfaces
DONE        2.4. Events about connections of unsupported network interfaces are rejected
DONE        2.5. Interface is added and removed from the list when receiving the corresponding events
DONE        2.6. Removal of interfaces added while reading the connected interfaces on init
DONE        2.7. Removal of connected interface doesn't affect other interfaces
DONE        2.8. Addon correctly reads the info about connected network interfaces

 */

class CameraAddonTestSuite : public Test {
public:
    using ConnectedNetworkIntefaces = CamerasAddon::ConnectedNetworkIntefaces;

    static void updateIpAddress( CamerasAddon& cameraAddon, const JsonObject& params ) {
        cameraAddon.updateIpAddress( params );
    }
    static const std::string& getCurrentIpAddress( CamerasAddon& cameraAddon ) {
        return cameraAddon.m_currentIpAddress;
    }

    static ConnectedNetworkIntefaces& getConnectedNetworkInterfaces( CamerasAddon& cameraAddon ) {
        return cameraAddon.m_connectedNetworkInterfaces;
    }
    static void handleNetworkInterfaceStatusChanged( CamerasAddon& cameraAddon,
                                                     const std::string& interfaceId,
                                                     const std::string& interfaceType,
                                                     bool isConnected ) {
        const Json params =
            JSON( "interfaceId" << interfaceId << "type" << interfaceType << "connected" << isConnected );
        cameraAddon.handleNetworkInterfaceStatusChanged( params );
    }
    static void checkConnectedNetworkInterfaceChange( CamerasAddon& cameraAddon,
                                                      const std::string& interfaceId,
                                                      const std::string& interfaceType,
                                                      bool isConnected ) {
        const ConnectedNetworkIntefaces& interfaces = getConnectedNetworkInterfaces( cameraAddon );
        const int interfacesCountBefore = static_cast< int >( interfaces.size() );

        handleNetworkInterfaceStatusChanged( cameraAddon, interfaceId, interfaceType, isConnected );

        const int expectedInterfacesCount = isConnected ? interfacesCountBefore + 1 : interfacesCountBefore - 1;
        EXPECT_EQ( expectedInterfacesCount, static_cast< int >( interfaces.size() ) );

        auto it = interfaces.find( interfaceId );
        if( isConnected )
            EXPECT_TRUE( it != interfaces.end() );
        else
            EXPECT_TRUE( it == interfaces.end() );
    }

    static void readConnectedNetworkInterfaces( CamerasAddon& cameraAddon, const std::vector< Json >& interfaces ) {
        return cameraAddon.readConnectedNetworkInterfaces( interfaces );
    }
};

TEST_F( CameraAddonTestSuite, updateIpAddress ) {  // 1.1.
    const Json ipAddressString = R"(
    {
        "ipv4": "192.168.0.1",
        "type": "ethernet"
    }
    )"_json;

    const Json ipAddressObject = R"(
    {
        "ipv4": {
            "ip": "192.168.0.2"
        },
        "type": "ethernet"
    }
    )"_json;

    const Json ipAddressModemType = R"(
    {
        "ipv4": "192.168.0.3",
        "type": "modem"
    }
    )"_json;

    CamerasAddon cameraAddon;

    updateIpAddress( cameraAddon, ipAddressString );
    EXPECT_EQ( getCurrentIpAddress( cameraAddon ), "192.168.0.1" );

    updateIpAddress( cameraAddon, ipAddressObject );
    EXPECT_EQ( getCurrentIpAddress( cameraAddon ), "192.168.0.2" );

    updateIpAddress( cameraAddon, ipAddressString );
    EXPECT_EQ( getCurrentIpAddress( cameraAddon ), "192.168.0.1" );

    updateIpAddress( cameraAddon, ipAddressObject );
    EXPECT_EQ( getCurrentIpAddress( cameraAddon ), "192.168.0.2" );

    updateIpAddress( cameraAddon, ipAddressModemType );
    EXPECT_EQ( getCurrentIpAddress( cameraAddon ), "192.168.0.2" );
}

TEST_F( CameraAddonTestSuite, ethernetConnectionAdded ) {  // 2.1.
    CamerasAddon cameraAddon;
    checkConnectedNetworkInterfaceChange( cameraAddon, "interfaceId", Network::Resources::EthernetType, true );
}

TEST_F( CameraAddonTestSuite, wifiConnectionAdded ) {  // 2.2.
    CamerasAddon cameraAddon;
    checkConnectedNetworkInterfaceChange( cameraAddon, "interfaceId", Network::Resources::WifiType, true );
}

TEST_F( CameraAddonTestSuite, multipleConnectionTypesAdded ) {  // 2.3.
    CamerasAddon cameraAddon;
    checkConnectedNetworkInterfaceChange( cameraAddon, "interfaceIdEthernet", Network::Resources::EthernetType, true );
    checkConnectedNetworkInterfaceChange( cameraAddon, "interfaceIdWifi", Network::Resources::WifiType, true );
}

TEST_F( CameraAddonTestSuite, unsupportedConnectionTypeRejected ) {  // 2.4.
    auto testFun = []( CamerasAddon& cameraAddon, const std::string& interfaceId, const std::string& interfaceType ) {
        const ConnectedNetworkIntefaces& interfaces = getConnectedNetworkInterfaces( cameraAddon );
        const size_t connectedInterfaceCountBefore = interfaces.size();

        handleNetworkInterfaceStatusChanged( cameraAddon, interfaceId, interfaceType, true );

        EXPECT_EQ( connectedInterfaceCountBefore, interfaces.size() );
        auto it = interfaces.find( interfaceId );
        EXPECT_TRUE( it == interfaces.end() );
    };

    CamerasAddon cameraAddon;
    testFun( cameraAddon, "testInterfaceId", Network::Resources::MobileType );
    testFun( cameraAddon, "testInterfaceId", Network::Resources::ClientType );
}

TEST_F( CameraAddonTestSuite, addAndRemoveNetworkConnection ) {  // 2.5.
    CamerasAddon cameraAddon;
    const std::string interfaceId = "interfaceId";
    handleNetworkInterfaceStatusChanged( cameraAddon, interfaceId, Network::Resources::WifiType, true );
    handleNetworkInterfaceStatusChanged( cameraAddon, interfaceId, Network::Resources::WifiType, false );

    const ConnectedNetworkIntefaces& interfaces = getConnectedNetworkInterfaces( cameraAddon );
    EXPECT_TRUE( interfaces.empty() );
}

TEST_F( CameraAddonTestSuite, removeAutoDetectedNetworkConnection ) {  // 2.6.
    const std::string interfaceId = "interfaceId";

    CamerasAddon cameraAddon;
    ConnectedNetworkIntefaces& interfaces = getConnectedNetworkInterfaces( cameraAddon );
    interfaces.insert( interfaceId );
    handleNetworkInterfaceStatusChanged( cameraAddon, interfaceId, Network::Resources::WifiType, false );

    EXPECT_TRUE( interfaces.empty() );
}

TEST_F( CameraAddonTestSuite, addAndRemoveNetworkConnectionDoesntAffectOtherNetworks ) {  // 2.7.
    const std::string interfaceIdWifi = "interfaceIdWifi";
    const std::string interfaceIdEthernet = "interfaceIdEthernet";

    CamerasAddon cameraAddon;
    handleNetworkInterfaceStatusChanged( cameraAddon, interfaceIdWifi, Network::Resources::WifiType, true );
    handleNetworkInterfaceStatusChanged( cameraAddon, interfaceIdEthernet, Network::Resources::EthernetType, true );
    handleNetworkInterfaceStatusChanged( cameraAddon, interfaceIdWifi, Network::Resources::WifiType, false );

    const ConnectedNetworkIntefaces& interfaces = getConnectedNetworkInterfaces( cameraAddon );
    auto it = interfaces.find( interfaceIdEthernet );
    EXPECT_TRUE( it != interfaces.end() );
}

TEST_F( CameraAddonTestSuite, readingConnectedNetworkInterfaces ) {  // 2.8.
    const std::vector< std::string > interfacesIds = { "interfaceIdWifi1", "interfaceIdWifi2", "interfaceIdEthernet" };
    const JsonArray connectedInterfaces = R"(
    [
        {
            "_id": "interfaceIdEthernet",
            "enabled": "on",
            "hwaddr": "02:42:44:3f:a6:3d",
            "internetAvailable": true,
            "ipv4": {
                "dns": [
                    "192.168.10.10",
                    "1.1.1.1",
                    "8.8.8.8"
                ],
                "gateway": "192.168.10.1",
                "ip": "192.168.10.44",
                "mask": "255.255.254.0",
                "mode": "dhcp"
            },
            "network": "wan",
            "priority": 0,
            "status": "up",
            "type": "ethernet"
        },
        {
            "_id": "interfaceIdWifi1",
            "enabled": "auto",
            "internetAvailable": false,
            "ipv4": {
                "mode": "dhcp"
            },
            "network": "wan",
            "priority": 1,
            "status": "down",
            "type": "wifi",
            "wifi": {}
        },
        {
            "_id": "interfaceIdWifi2",
            "enabled": "off",
            "ipv4": {
                "ip": "172.27.64.20",
                "mask": "255.255.255.0",
                "mode": "static"
            },
            "network": "lan",
            "priority": 2,
            "status": "down",
            "type": "wifi",
            "wifi": {
                "channel": "11",
                "mode": "ap",
                "network": {
                    "encryption": "psk2",
                    "key": "a7a5074f",
                    "ssid": "wifi_90001797"
                },
                "region": "00"
            }
        }
    ] )"_json;

    std::vector< Json > connectedInterfacesVec;
    connectedInterfacesVec.reserve( connectedInterfaces.size() );
    for( const Json& element : connectedInterfaces ) {
        connectedInterfacesVec.push_back( element );
    }

    CamerasAddon cameraAddon;
    readConnectedNetworkInterfaces( cameraAddon, connectedInterfacesVec );

    const ConnectedNetworkIntefaces& interfaces = getConnectedNetworkInterfaces( cameraAddon );
    EXPECT_EQ( interfaces.size(), interfacesIds.size() );

    for( const std::string& interfaceId : interfacesIds ) {
        auto it = interfaces.find( interfaceId );
        EXPECT_TRUE( it != interfaces.end() );
    }
}

}  // namespace UT
}  // namespace Cameras
}  // namespace HubPlatform
