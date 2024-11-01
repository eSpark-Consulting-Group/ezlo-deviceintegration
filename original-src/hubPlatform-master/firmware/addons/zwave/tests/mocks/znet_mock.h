#pragma once

#include "zwave/tests/mocks/znet_provisioning_list_controller_mock.h"
#include "zwave/tests/mocks/znet_mock_command_statistics_collector.h"

#include <znet/iznet.hpp>
#include <znet/iznet_owner.hpp>
#include <znet/znet_default_timer_provider.hpp>
#include <znet/znet_requests_manager.hpp>
#include <znet/znet_reachable_check_timeout_settings.hpp>
#include <znet/iznet_provisioning_list_provider.hpp>
#include <znet/iznet_provisioning_list_observer.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Zwave {
namespace UT {

class ZNetMock : public znet::IZNet, public znet::DefaultTimerProvider, public MockCommandStatisticsCollector {
public:
    ZNetMock()
    : m_requestsManager( *this, *this ) {}
    ZNetMock( znet::IProvisioningListProvider&,
              znet::IProvisioningListProvider&,
              znet::IProvisioningListObserver&,
              znet::IZNetOwner& )
    : m_requestsManager( *this, *this ) {}

    MOCK_METHOD1( resetZwaveModule, void( bool ) );
    MOCK_METHOD0( initZnet, void() );
    MOCK_METHOD1( startExclude, void( uint8_t ) );
    MOCK_METHOD0( startExclude, void() );
    MOCK_METHOD0( stopExclude, void() );
    MOCK_METHOD0( startInclude, void() );
    MOCK_METHOD0( stopInclude, void() );
    MOCK_METHOD0( startLearn, void() );
    MOCK_METHOD0( stopLearn, void() );
    MOCK_METHOD0( reconstructNetwork, void() );
    MOCK_METHOD0( getBackgroundRSSI, void() );
    MOCK_METHOD0( stopSecurityInclude, void() );
    MOCK_METHOD0( sendNif, void() );
    MOCK_METHOD1( requestNif, bool( uint8_t ) );
    MOCK_METHOD1( sendNop, bool( uint8_t ) );
    MOCK_METHOD1( setPublicKey, void( znet_S2_pub_key_t* ) );
    MOCK_METHOD1( s2SelectKeys, void( znet_S2_keys_t ) );
    MOCK_CONST_METHOD0( isIncludeExcludeAvailable, bool() );
    MOCK_METHOD0( isResetting, bool() );
    MOCK_METHOD0( isBusy, bool() );
    MOCK_METHOD0( onLoop, bool() );
    MOCK_METHOD6( setConfigParameter,
                  bool( const uint8_t, const uint8_t, const uint8_t, const uint32_t, const uint8_t, const int ) );
    MOCK_METHOD3( getConfigParameter, bool( const uint8_t, const uint8_t, const uint8_t ) );
    MOCK_METHOD1( getAssociationGroups, bool( const uint8_t ) );
    MOCK_METHOD1( requestAllNodesValues, void( bool ) );
    MOCK_METHOD5( requestNodeValues, void( const uint8_t, std::chrono::milliseconds, bool, bool, size_t ) );
    MOCK_METHOD0( updateTimeSettings, void() );
    MOCK_METHOD1( interviewNode, void( const uint8_t ) );
    MOCK_METHOD1( synchronizeNodeTime, void( const uint8_t ) );

    MOCK_METHOD1( checkReachableStatus, void( const uint8_t ) );
    MOCK_METHOD3( checkNodeValues,
                  void( const uint8_t, std::chrono::milliseconds, std::vector< std::pair< uint8_t, uint8_t > > ) );

    MOCK_METHOD6( addNodeToGroup,
                  znet::AssociationAddResult( znet_node_id_t,
                                              znet_node_channel_id_t,
                                              znet_grouping_id_t,
                                              znet_node_id_t,
                                              znet_node_channel_id_t,
                                              bool ) );
    MOCK_METHOD5(
        removeNodeFromGroup,
        bool( znet_node_id_t, znet_node_channel_id_t, znet_grouping_id_t, znet_node_id_t, znet_node_channel_id_t ) );
    MOCK_METHOD3( setupCommandClassAssociation,
                  znet::AssociationAddResult( znet_node_id_t,
                                              znet_command_class_t,
                                              std::function< void( znet::RequestFailTypeOpt ) > ) );

    MOCK_METHOD3( updateNodeTargetFirmware, void( znet_node_id_t, uint8_t, znet::FirmwareUpdate::SourceUPtr ) );

    MOCK_METHOD4( updateReachableCheckTimeoutSettings, void( bool, uint32_t, uint32_t, uint8_t ) );
    MOCK_METHOD1( updateUnresponsiveDeviceIgnoreInterval, void( uint32_t ) );

    znet::RequestsManager& getRequestsManager() override {
        return m_requestsManager;
    }

    znet::ReachableCheckTimeoutSettings& getReachableCheckTimeoutSettings() override {
        return m_reachableCheckTimeoutSettings;
    }

    znet::NodeControlManager& getNodeControlManager() override {
        throw std::runtime_error( "znet::NodeControlManager is not implemented in tests" );
    }

    znet::CommandClassValuesSetter& getCommandClassValuesSetter() override {
        throw std::runtime_error( "znet::CommandClassValuesSetter is not implemented in tests" );
    }

    std::unique_ptr< znet::CommonInfo > getCommonInfo() override {
        throw std::runtime_error( "znet::getCommonInfo is not implemented in tests" );
    }

    ::znet::api::IProvisioningListController& getProvisioningListController() override {
        return m_provisioningListControllerMock;
    }

    ::znet::api::IProvisioningListQueryDataSource& getProvisioningListControllerDataSource() override {
        throw std::runtime_error( "znet::getProvisioningListControllerDataSource is not implemented in tests" );
    }

    MOCK_METHOD1( createChipBackup, bool( znet::IOWrapper::DestinationUPtr ) );

    MOCK_METHOD1( restoreChipBackup, bool( znet::IOWrapper::SourceUPtr ) );

private:
    znet::RequestsManager m_requestsManager;
    znet::ReachableCheckTimeoutSettings m_reachableCheckTimeoutSettings;
    ProvisioningListControllerMock m_provisioningListControllerMock;
};

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
