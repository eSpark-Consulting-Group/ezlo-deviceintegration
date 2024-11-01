#include "zwave/zwave_node_statistics_collector.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Single delay
DONE    2. Multiple delays
DONE    3. Single fail
DONE    4. Multiple fails
DONE    5. Mixed
DONE    6. Buffer overflow
DONE    7. Empty buffer


*/

namespace HubPlatform {
namespace Zwave {
namespace UT {

using namespace ::testing;

class NodeStatisticsCollectorTestSuite : public Test {
protected:
    void SetUp() override {
        reset( 4 );
    }

    void reset( size_t bufferSize ) {
        m_collector = std::make_unique< NodeStatisticsCollector >( bufferSize );
    }

    void checkStatistics( std::vector< NodeStatisticsCollector::CommandInfo > expected ) {
        int counter = 0;
        m_collector->fetchStatisctics( [&]( NodeStatisticsCollector::CommandInfo info ) {
            ASSERT_LT( counter, expected.size() );
            auto expectedInfo = expected.at( counter );

            EXPECT_EQ( info.m_nodeId, expectedInfo.m_nodeId );
            EXPECT_EQ( info.m_status, expectedInfo.m_status );
            EXPECT_EQ( info.m_commandSent, expectedInfo.m_commandSent );
            EXPECT_EQ( info.m_delay, expectedInfo.m_delay );

            ++counter;
        } );

        EXPECT_EQ( counter, expected.size() );

        // Check that the buffer is empty
        m_collector->fetchStatisctics( [&]( NodeStatisticsCollector::CommandInfo ) { ++counter; } );

        EXPECT_EQ( counter, expected.size() );
    }

    std::unique_ptr< NodeStatisticsCollector > m_collector;
};

TEST_F( NodeStatisticsCollectorTestSuite, testSingleDelay /*1*/ ) {
    using Status = NodeStatisticsCollector::CommandInfo::Status;

    uint16_t nodeId = 5;
    auto timepoint = std::chrono::system_clock::now();
    auto duration = std::chrono::milliseconds{ 20 };
    m_collector->onNodeCommandDelay( nodeId, timepoint, duration );

    checkStatistics( { { nodeId, Status::Sent, timepoint, duration } } );
}

TEST_F( NodeStatisticsCollectorTestSuite, testMultipleDelays /*2*/ ) {
    using Status = NodeStatisticsCollector::CommandInfo::Status;

    uint16_t nodeId1 = 5;
    auto timepoint1 = std::chrono::system_clock::now();
    auto duration1 = std::chrono::milliseconds{ 20 };

    uint16_t nodeId2 = 6;
    auto timepoint2 = std::chrono::system_clock::now() + std::chrono::seconds{ 30 };
    auto duration2 = std::chrono::milliseconds{ 30 };
    m_collector->onNodeCommandDelay( nodeId1, timepoint1, duration1 );
    m_collector->onNodeCommandDelay( nodeId2, timepoint2, duration2 );

    checkStatistics(
        { { nodeId1, Status::Sent, timepoint1, duration1 }, { nodeId2, Status::Sent, timepoint2, duration2 } } );
}

TEST_F( NodeStatisticsCollectorTestSuite, testSingleFail /*3*/ ) {
    using Status = NodeStatisticsCollector::CommandInfo::Status;

    uint16_t nodeId = 5;
    auto timepoint = std::chrono::system_clock::now();
    m_collector->onNodeCommandFailed( nodeId, timepoint );

    checkStatistics( { { nodeId, Status::Failed, timepoint } } );
}

TEST_F( NodeStatisticsCollectorTestSuite, testMultipleFails /*4*/ ) {
    using Status = NodeStatisticsCollector::CommandInfo::Status;

    uint16_t nodeId1 = 5;
    auto timepoint1 = std::chrono::system_clock::now();

    uint16_t nodeId2 = 6;
    auto timepoint2 = std::chrono::system_clock::now() + std::chrono::seconds{ 30 };
    m_collector->onNodeCommandFailed( nodeId1, timepoint1 );
    m_collector->onNodeCommandFailed( nodeId2, timepoint2 );

    checkStatistics( { { nodeId1, Status::Failed, timepoint1 }, { nodeId2, Status::Failed, timepoint2 } } );
}

TEST_F( NodeStatisticsCollectorTestSuite, testMixed /*5*/ ) {
    using Status = NodeStatisticsCollector::CommandInfo::Status;

    uint16_t nodeId1 = 5;
    auto timepoint1 = std::chrono::system_clock::now();
    auto duration1 = std::chrono::milliseconds{ 20 };

    uint16_t nodeId2 = 6;
    auto timepoint2 = std::chrono::system_clock::now() + std::chrono::seconds{ 5 };

    uint16_t nodeId3 = 7;
    auto timepoint3 = std::chrono::system_clock::now() + std::chrono::seconds{ 10 };
    auto duration3 = std::chrono::milliseconds{ 30 };

    uint16_t nodeId4 = 8;
    auto timepoint4 = std::chrono::system_clock::now() + std::chrono::seconds{ 15 };

    m_collector->onNodeCommandDelay( nodeId1, timepoint1, duration1 );
    m_collector->onNodeCommandFailed( nodeId2, timepoint2 );
    m_collector->onNodeCommandDelay( nodeId3, timepoint3, duration3 );
    m_collector->onNodeCommandFailed( nodeId4, timepoint4 );

    checkStatistics( { { nodeId1, Status::Sent, timepoint1, duration1 },
                       { nodeId2, Status::Failed, timepoint2 },
                       { nodeId3, Status::Sent, timepoint3, duration3 },
                       { nodeId4, Status::Failed, timepoint4 } } );

    m_collector->onNodeCommandDelay( nodeId3, timepoint3, duration3 );
    m_collector->onNodeCommandFailed( nodeId2, timepoint2 );

    checkStatistics( { { nodeId3, Status::Sent, timepoint3, duration3 }, { nodeId2, Status::Failed, timepoint2 } } );
}

TEST_F( NodeStatisticsCollectorTestSuite, testBufferOwerflow /*6*/ ) {
    using Status = NodeStatisticsCollector::CommandInfo::Status;

    reset( 3 );

    uint16_t nodeId1 = 5;
    auto timepoint1 = std::chrono::system_clock::now();
    auto duration1 = std::chrono::milliseconds{ 20 };

    uint16_t nodeId2 = 6;
    auto timepoint2 = std::chrono::system_clock::now() + std::chrono::seconds{ 5 };

    uint16_t nodeId3 = 7;
    auto timepoint3 = std::chrono::system_clock::now() + std::chrono::seconds{ 10 };
    auto duration3 = std::chrono::milliseconds{ 30 };

    uint16_t nodeId4 = 8;
    auto timepoint4 = std::chrono::system_clock::now() + std::chrono::seconds{ 15 };

    m_collector->onNodeCommandDelay( nodeId1, timepoint1, duration1 );
    m_collector->onNodeCommandFailed( nodeId2, timepoint2 );
    m_collector->onNodeCommandDelay( nodeId3, timepoint3, duration3 );
    m_collector->onNodeCommandFailed( nodeId4, timepoint4 );

    checkStatistics( { { nodeId2, Status::Failed, timepoint2 },
                       { nodeId3, Status::Sent, timepoint3, duration3 },
                       { nodeId4, Status::Failed, timepoint4 } } );
}

TEST_F( NodeStatisticsCollectorTestSuite, testEmptyBuffer /*7*/ ) {
    using Status = NodeStatisticsCollector::CommandInfo::Status;

    checkStatistics( {} );
}

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
