#include "zwave/zwave_node_manager.h"
#include "zwave/resources/zwave_resources.h"
#include "zwave/common/zwave_exceptions.h"

#include "database_client/fake_db/fake_db_client.h"
#include "json_utils/json_utils.h"

#include "common_utils/cpp14.h"

#include "zwave/tests/mocks/znet_mock.h"

#include <znet/znet_node.hpp>
#include <znet/iassociations_configurator.h>
#include <znet/znet_requests_manager.hpp>
#include <znet/znet_reachable_check_timeout_settings.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. Register
DONE        1.1 One node
DONE        1.2 Several nodes
DONE        1.3 With the same parameters several times
DONE    2. Remove
DONE        2.1 One node
DONE        2.2 Several nodes
DONE        2.3 The same node several times
DONE    3. Update
DONE        3.1 One node
DONE        3.2 Several nodes
DONE        3.3 The same node several times
DONE    4. Reset
DONE        4.1 Empty manager
DONE        4.2 Only one node
DONE        4.2 Several nodes
DONE    5. Finding
DONE        5.1 Existing node
DONE        5.2 Non existing node
DONE    6. Node manager's loading

*/

namespace HubPlatform {
namespace Zwave {
namespace UT {

class NodeManagerTestSuite : public ::testing::Test,
                             public znet::ITimerProvider,
                             public MockCommandStatisticsCollector {
public:
    void SetUp() override {
        m_dbClient = std::make_unique< HubPlatform::UT::FakeDbClient >();
        m_requestsRepeater = std::make_unique< znet::RequestsManager >( *this, *this );
        m_reachableCheckTimeoutSettings = std::make_unique< znet::ReachableCheckTimeoutSettings >();
        m_znet = std::make_unique< ZNetMock >();
        m_nodeManager = std::make_unique< NodeManager >(
            *m_dbClient, *m_requestsRepeater, *m_znet, *m_reachableCheckTimeoutSettings );
    }

    void TearDown() override {
        m_nodeManager.reset();
        m_requestsRepeater.reset();
        m_znet.reset();
        m_dbClient.reset();
    }

    IDbClient& getDBClient() {
        return *m_dbClient;
    }

    bool containsInDB( uint8_t nodeId ) const {
        return m_dbClient->count( Resources::DB::TableNames::Devices,
                                  JSON( Resources::DB::Fields::Id << std::to_string( nodeId1 ) ) );
    }

    size_t getDeviceCount() const {
        return m_dbClient->count( Resources::DB::TableNames::Devices, JSON_EMPTY_OBJ );
    }

    NodeManager& getManager() {
        return *m_nodeManager;
    }

    TimerUPtr createTimer( TimerCallback ) override {
        return nullptr;
    }

    const uint8_t nodeId1{ 1 };
    const uint8_t genericType1{ 1 };
    const uint8_t basicType1{ 1 };
    const uint8_t specificType1{ 1 };

    const uint8_t nodeId2{ 2 };
    const uint8_t genericType2{ 2 };
    const uint8_t basicType2{ 2 };
    const uint8_t specificType2{ 2 };

private:
    std::unique_ptr< IDbClient > m_dbClient;
    std::unique_ptr< znet::RequestsManager > m_requestsRepeater;
    std::unique_ptr< znet::ReachableCheckTimeoutSettings > m_reachableCheckTimeoutSettings;
    std::unique_ptr< ZNetMock > m_znet;
    std::unique_ptr< NodeManager > m_nodeManager;
};

TEST_F( NodeManagerTestSuite, testRegisterOneNode /*1.1*/ ) {
    auto& node = getManager().registerNode( nodeId1 );

    EXPECT_EQ( node.getId(), nodeId1 );
    EXPECT_TRUE( containsInDB( nodeId1 ) );
}

TEST_F( NodeManagerTestSuite, testRegisterSeveralNodes /*1.2*/ ) {
    auto& node1 = getManager().registerNode( nodeId1 );
    auto& node2 = getManager().registerNode( nodeId2 );

    EXPECT_EQ( node1.getId(), nodeId1 );
    EXPECT_EQ( node2.getId(), nodeId2 );

    EXPECT_TRUE( containsInDB( nodeId1 ) );
    EXPECT_TRUE( containsInDB( nodeId2 ) );
}

TEST_F( NodeManagerTestSuite, testRegisterWithTheSameParametersSeveralTimes /*1.3*/ ) {
    getManager().registerNode( nodeId1 );
    auto& node = getManager().registerNode( nodeId1 );

    EXPECT_EQ( node.getId(), nodeId1 );

    EXPECT_EQ( getDeviceCount(), 1 );
    EXPECT_TRUE( containsInDB( nodeId1 ) );
}

TEST_F( NodeManagerTestSuite, testRemoveOneNode /*2.1*/ ) {
    getManager().registerNode( nodeId1 );

    EXPECT_TRUE( getManager().remove( nodeId1 ) );

    EXPECT_FALSE( containsInDB( nodeId1 ) );
}

TEST_F( NodeManagerTestSuite, testRemoveSeveralNodes /*2.2*/ ) {
    getManager().registerNode( nodeId1 );
    getManager().registerNode( nodeId2 );

    EXPECT_TRUE( getManager().remove( nodeId1 ) );
    EXPECT_TRUE( getManager().remove( nodeId2 ) );

    EXPECT_FALSE( containsInDB( nodeId1 ) );
    EXPECT_FALSE( containsInDB( nodeId2 ) );
}

TEST_F( NodeManagerTestSuite, testRemoveTheSameNodeSeveralTimes /*2.3*/ ) {
    getManager().registerNode( nodeId1 );

    EXPECT_TRUE( getManager().remove( nodeId1 ) );
    EXPECT_FALSE( getManager().remove( nodeId1 ) );

    EXPECT_FALSE( containsInDB( nodeId1 ) );
}

TEST_F( NodeManagerTestSuite, testUpdateOneNode /*3.1*/ ) {
    using namespace Resources;
    using namespace Fields;

    auto& node = getManager().registerNode( nodeId1 );
    node.setNodeInfoExtTypes( genericType1, basicType1, specificType1 );

    getManager().update( nodeId1 );

    auto otherNode = getManager().deserializeNode(
        getDBClient().findOne( DB::TableNames::Devices, JSON( "_id" << std::to_string( nodeId1 ) ), nullptr ),
        nullptr,
        nullptr,
        nullptr );
    EXPECT_EQ( otherNode->getId(), nodeId1 );
    EXPECT_EQ( otherNode->getGenericType(), genericType1 );
    EXPECT_EQ( otherNode->getBasicType(), basicType1 );
    EXPECT_EQ( otherNode->getGenericType(), specificType1 );
}

TEST_F( NodeManagerTestSuite, testUpdateSeveralNodes /*3.2*/ ) {
    using namespace Resources;
    using namespace Fields;

    auto& node1 = getManager().registerNode( nodeId1 );
    auto& node2 = getManager().registerNode( nodeId2 );

    node1.setNodeInfoExtTypes( genericType2, basicType2, specificType2 );
    node2.setNodeInfoExtTypes( genericType1, basicType1, specificType1 );

    getManager().update( nodeId1 );
    getManager().update( nodeId2 );

    auto otherNode1 = getManager().deserializeNode(
        getDBClient().findOne( DB::TableNames::Devices, JSON( "_id" << std::to_string( nodeId1 ) ), nullptr ),
        nullptr,
        nullptr,
        nullptr );
    EXPECT_EQ( otherNode1->getId(), nodeId1 );
    EXPECT_EQ( otherNode1->getGenericType(), genericType2 );
    EXPECT_EQ( otherNode1->getBasicType(), basicType2 );
    EXPECT_EQ( otherNode1->getGenericType(), specificType2 );

    auto otherNode2 = getManager().deserializeNode(
        getDBClient().findOne( DB::TableNames::Devices, JSON( "_id" << std::to_string( nodeId2 ) ), nullptr ),
        nullptr,
        nullptr,
        nullptr );
    EXPECT_EQ( otherNode2->getId(), nodeId2 );
    EXPECT_EQ( otherNode2->getGenericType(), genericType1 );
    EXPECT_EQ( otherNode2->getBasicType(), basicType1 );
    EXPECT_EQ( otherNode2->getGenericType(), specificType1 );
}

TEST_F( NodeManagerTestSuite, testUpdateTheSameNodeSeveralTimes /*3.3*/ ) {
    using namespace Resources;
    using namespace Fields;

    getManager().registerNode( nodeId1 );

    getManager().findNode( nodeId1 )->setNodeInfoExtTypes( genericType2, basicType2, specificType2 );
    getManager().update( nodeId1 );

    getManager().findNode( nodeId1 )->setNodeInfoExtTypes( genericType1, basicType2, specificType1 );
    getManager().update( nodeId1 );

    auto otherNode = getManager().deserializeNode(
        getDBClient().findOne( DB::TableNames::Devices, JSON( "_id" << std::to_string( nodeId1 ) ), nullptr ),
        nullptr,
        nullptr,
        nullptr );
    EXPECT_EQ( otherNode->getId(), nodeId1 );
    EXPECT_EQ( otherNode->getGenericType(), genericType1 );
    EXPECT_EQ( otherNode->getBasicType(), basicType2 );
    EXPECT_EQ( otherNode->getGenericType(), specificType1 );
}

TEST_F( NodeManagerTestSuite, testResetEmptyManager /*4.1*/ ) {
    getManager().reset();

    EXPECT_EQ( getDeviceCount(), 0 );
}

TEST_F( NodeManagerTestSuite, testResetOnlyOneNode /*4.2*/ ) {
    getManager().registerNode( nodeId1 );

    EXPECT_EQ( getDeviceCount(), 1 );

    getManager().reset();

    EXPECT_EQ( getDeviceCount(), 0 );
    EXPECT_FALSE( getManager().findNode( nodeId1 ) );
}

TEST_F( NodeManagerTestSuite, testResetSeveralNodes /*4.3*/ ) {
    getManager().registerNode( nodeId1 );
    getManager().registerNode( nodeId2 );

    EXPECT_EQ( getDeviceCount(), 2 );

    getManager().reset();

    EXPECT_EQ( getDeviceCount(), 0 );
    EXPECT_FALSE( getManager().findNode( nodeId1 ) );
    EXPECT_FALSE( getManager().findNode( nodeId2 ) );
}

TEST_F( NodeManagerTestSuite, testGettingExistingNode /*5.1*/ ) {
    getManager().registerNode( nodeId1 );

    auto node = getManager().findNode( nodeId1 );

    ASSERT_TRUE( node );
    EXPECT_TRUE( containsInDB( node->getId() ) );
}

TEST_F( NodeManagerTestSuite, testGettingNonExistingNode /*5.2*/ ) {
    EXPECT_FALSE( getManager().findNode( nodeId1 ) );
}

TEST_F( NodeManagerTestSuite, testNodeManagerLoading /*6.1*/ ) {
    using namespace Resources;

    constexpr auto security = znet::SecurityMode::Class::S2_Authenticated;
    auto node = std::make_unique< znet::ZNode >( nullptr, nullptr, nullptr );
    node->init( nodeId1, security );

    auto& db = getDBClient();
    db.insert( DB::TableNames::Devices, NodeManager::serializeNode( *node ) );

    auto resultNode = getManager().findNode( nodeId1 );

    EXPECT_EQ( resultNode->getId(), nodeId1 );
    EXPECT_EQ( resultNode->getSecurity(), security );
}

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
