#include "watcher/issues_monitor/handlers/zwave_node_expired_handler.h"

#include "watcher/resources/watcher_resources.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

#include <future>

/*

TESTPLAN

DONE    1. ZWave node expired tests
DONE        1.1 Restart zwave service

*/

namespace HubPlatform {
namespace UT {

class ZWaveNodeExpiredHandlerTestSuite : public ::testing::Test {
public:
    class FakeImpl : public HubPlatform::Watcher::IssueHandlers::ZWaveNodeExpiredHandler::Impl {
    public:
        Watcher::Types::ZWaveNodeStatus getZWaveNodeStatus() override {
            return m_zwaveNodeStatus;
        }

        void restartService( const std::string& serviceName ) override {
            m_restartedProcesses.emplace( serviceName );
        }

        Watcher::Types::ZWaveNodeStatus m_zwaveNodeStatus;
        std::set< std::string > m_restartedProcesses;
    };
};

TEST_F( ZWaveNodeExpiredHandlerTestSuite, restartZWaveService ) /* 1.1 */ {
    auto implPtr = std::make_unique< FakeImpl >();
    auto& impl = *implPtr;

    const std::chrono::milliseconds timeout( 10 );
    Watcher::IssueHandlers::ZWaveNodeExpiredHandler handler( std::move( implPtr ) );
    handler.setZWaveNodeExpirationTimeout( timeout );

    // Set normal status
    impl.m_zwaveNodeStatus = Watcher::Types::ZWaveNodeStatus::ZWAVE_NODE_NOT_EXPIRED;

    // No expiration on first run
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_restartedProcesses.size(), 0 );

    std::this_thread::sleep_for( timeout + std::chrono::milliseconds( 10 ) );

    // No expiration for correct status
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_restartedProcesses.size(), 0 );

    // Set expired status
    impl.m_zwaveNodeStatus = Watcher::Types::ZWaveNodeStatus::ZWAVE_NODE_EXPIRED;

    // No expiration on status change
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_restartedProcesses.size(), 0 );

    std::this_thread::sleep_for( timeout + std::chrono::milliseconds( 10 ) );

    // Node expired
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_restartedProcesses, std::set< std::string >( { Watcher::Resources::ServiceNames::ZWave } ) );
    impl.m_restartedProcesses.clear();

    // Set normal status
    impl.m_zwaveNodeStatus = Watcher::Types::ZWaveNodeStatus::ZWAVE_NODE_NOT_EXPIRED;
    std::this_thread::sleep_for( timeout + std::chrono::milliseconds( 10 ) );

    // No expiration for correct status
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_restartedProcesses.size(), 0 );
}

}  // namespace UT
}  // namespace HubPlatform
