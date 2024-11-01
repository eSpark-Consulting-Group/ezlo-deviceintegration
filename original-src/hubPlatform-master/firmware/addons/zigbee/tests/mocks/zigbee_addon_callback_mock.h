#pragma once

#include "zigbee/zigbee_addon_callback.h"
#include "zigbee/backup/zigbee_network_info.h"
#include "zigbee/interview_configurations/node_interview_configurations.h"
#include "zigbee/interview_configurations/node_interview_names_extractors.h"
#include "zigbee/model/data_structures/child_and_route_structures.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Zigbee {
namespace UT {

class ZigbeeAddonCallbackMock : public ZigbeeAddonCallbackInterface {
public:
    ZigbeeAddonCallbackMock() {}

    const ZigbeeNode* findNode( uint16_t nodeId ) override {
        auto it = m_nodes.find( nodeId );
        return it != m_nodes.end() ? it->second.get() : nullptr;
    }

    ZigbeeNode* takeNode( uint16_t nodeId ) {
        auto it = m_nodes.find( nodeId );
        return it != m_nodes.end() ? it->second.get() : nullptr;
    }

    void addNode( uint16_t nodeId ) {
        m_nodes.emplace( nodeId, std::make_unique< ZigbeeNode >( nodeId, true, true, true ) );
    }

    MOCK_METHOD0( stackInitialized, void() );
    MOCK_METHOD1( nodeAdded, void( std::shared_ptr< ZigbeeNode > ) );
    MOCK_METHOD1( mfgInterviewFinished, void( std::shared_ptr< ZigbeeNode > ) );
    MOCK_METHOD1( nodeRemoved, void( const uint16_t ) );
    MOCK_METHOD1( nodeRejoined, void( uint16_t ) );
    MOCK_METHOD1( includeStatusChange, void( ModeStatus ) );
    MOCK_METHOD5( attributeValueUpdated, void( uint16_t, uint16_t, uint16_t, uint16_t, const std::string ) );
    MOCK_METHOD4( armCommandReceived, void( uint16_t, uint16_t, IasAceArmCommandMode, const std::string ) );
    MOCK_METHOD4( simpleCommandReceived, void( uint16_t, uint16_t, uint16_t, uint16_t ) );
    MOCK_METHOD6( doorLockGetUserResponseReceived,
                  void( uint16_t, uint16_t, uint16_t, UserStatus, UserType, const std::string& ) );
    MOCK_METHOD6(
        doorLockOperatingEventNotificationReceived,
        void( uint16_t, uint16_t, DoorLockEventSource, DoorLockOperationEvent, uint16_t, const std::string& ) );
    MOCK_METHOD8( doorLockProgrammingEventNotificationReceived,
                  void( uint16_t,
                        uint16_t,
                        DoorLockEventSource,
                        DoorLockProgrammingEvent,
                        uint16_t,
                        const std::string&,
                        UserType,
                        UserStatus ) );
    MOCK_METHOD2( updateDeviceReachable, void( uint16_t, bool ) );
    MOCK_METHOD1( includeDeviceProgress, void( uint8_t ) );
    MOCK_METHOD3( userCodesRequestProgress, void( uint16_t, uint16_t, uint8_t ) );
    MOCK_METHOD0( networkDown, void() );
    MOCK_METHOD0( networkUp, void() );
    MOCK_METHOD2( networkInfoCollected, void( const Backup::ZigbeeNetworkInfo&, bool ) );
    MOCK_METHOD0( restoreBackupCompleted, void() );
    MOCK_METHOD1( restoreBackupFailed, void( Backup::BackupRestoreStatus ) );
    MOCK_METHOD4( alertsNotificationOrResponseReceived,
                  void( uint16_t, uint8_t, const std::vector< AlertStructure >&, HubPlatform::Zigbee::AlertSource ) );
    MOCK_METHOD2( provideNodeInterviewConfigurations,
                  std::unique_ptr< NodeInterviewConfigurations >( const std::string&, const std::string& ) );
    MOCK_METHOD0( provideNodeInterviewNamesExtractors, std::unique_ptr< NodeInterviewNamesExtractors >() );
    MOCK_METHOD0( onNetworkChannelChanged, void() );
    MOCK_METHOD4( onChildAndRouteInfo,
                  void( uint16_t, std::vector< ChildData >, std::vector< NeighborData >, std::vector< RouteData > ) );

private:
    std::map< uint16_t, std::unique_ptr< ZigbeeNode > > m_nodes;
};

}  // namespace UT
}  // namespace Zigbee
}  // namespace HubPlatform
