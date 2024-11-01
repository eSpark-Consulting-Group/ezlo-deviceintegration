#include "watcher/issues_monitor/handlers/unresponsive_services_handler.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include <gtest/gtest.h>

#include <future>

/*

TESTPLAN

DONE    1. Unresponsive services tests
DONE        1.1 Recovery with initial 'D' state
DONE        1.2 Recovery without initial 'D' state

*/

namespace HubPlatform {
namespace UT {

class UnresponsiveServicesHandlerTestSuite : public ::testing::Test {
public:
    using ProcessInfoList = std::vector< Watcher::Types::ProcessInfo >;
    using ServiceHeartbeatInfoMap = std::map< std::string, Watcher::Types::ServiceHeartbeatInfo >;

    class FakeImpl : public HubPlatform::Watcher::IssueHandlers::UnresponsiveServicesHandler::Impl {
    public:
        ProcessInfoList readProcessInfoList() override {
            return m_processInfoList;
        }

        ServiceHeartbeatInfoMap readServiceHeartbeatInfoMap() override {
            return m_heartbeatMap;
        }

        void restartService( const std::string& serviceName ) override {
            m_restartedProcesses.emplace( serviceName );
        }

        void updateUnresponsiveProcessesSet( std::set< Watcher::Types::Pid > set ) override {
            m_unresponsiveProcessesSet = set;
        }

        ServiceHeartbeatInfoMap m_heartbeatMap;
        ProcessInfoList m_processInfoList;
        std::set< Watcher::Types::Pid > m_unresponsiveProcessesSet;
        std::set< std::string > m_restartedProcesses;

        void registerService( const std::string& serviceName, Watcher::Types::Pid pid ) {
            auto& heartbeat = m_heartbeatMap[ serviceName ];
            heartbeat.serviceName = serviceName;
            heartbeat.pid = pid;

            m_processInfoList.emplace_back();
            auto& processInfo = m_processInfoList.back();
            processInfo.pid = pid;
            processInfo.state = "S";
            processInfo.cmd = serviceName;
        }
    };

    std::chrono::steady_clock::time_point getTimepointInPast( std::chrono::milliseconds shift ) {
        return std::chrono::steady_clock::now() - shift;
    }
};

TEST_F( UnresponsiveServicesHandlerTestSuite, genericUsecase ) /* 1.1 */ {
    auto implPtr = std::make_unique< FakeImpl >();
    auto& impl = *implPtr;

    const std::string serviceNameFoo = "ha-foo";
    const Watcher::Types::Pid servicePidFoo = 1000;
    impl.registerService( serviceNameFoo, servicePidFoo );

    const std::string serviceNameBar = "ha-bar";
    const Watcher::Types::Pid servicePidBar = 2000;
    impl.registerService( serviceNameBar, servicePidBar );

    auto& heartbeatFoo = impl.m_heartbeatMap.at( serviceNameFoo );
    auto& heartbeatBar = impl.m_heartbeatMap.at( serviceNameBar );

    const std::chrono::milliseconds defaultTimeout( 20 );
    const std::chrono::milliseconds serviceBarTimeout( 50 );

    Watcher::IssueHandlers::UnresponsiveServicesHandler handler( std::move( implPtr ) );
    handler.setDefaultHeartbeatTimeout( defaultTimeout );
    handler.setPerProcessHeartbeatTimeout( { { serviceNameBar, serviceBarTimeout } } );

    // No timeout
    heartbeatFoo.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 5 ) );
    heartbeatBar.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 5 ) );
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet.size(), 0 );
    EXPECT_EQ( impl.m_restartedProcesses.size(), 0 );

    // Timeout for foo
    heartbeatFoo.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 25 ) );
    heartbeatBar.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 25 ) );
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet, std::set< Watcher::Types::Pid >( { servicePidFoo } ) );
    EXPECT_EQ( impl.m_restartedProcesses, std::set< std::string >( { serviceNameFoo } ) );

    // Timeout for foo and bar
    heartbeatFoo.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 150 ) );
    heartbeatBar.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 150 ) );
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet, std::set< Watcher::Types::Pid >( { servicePidFoo, servicePidBar } ) );
    EXPECT_EQ( impl.m_restartedProcesses, std::set< std::string >( { serviceNameFoo, serviceNameBar } ) );
    impl.m_restartedProcesses.clear();
}

TEST_F( UnresponsiveServicesHandlerTestSuite, disabledTimeouts ) /* 1.1 */ {
    auto implPtr = std::make_unique< FakeImpl >();
    auto& impl = *implPtr;

    const std::string serviceNameFoo = "ha-foo";
    const Watcher::Types::Pid servicePidFoo = 1000;
    impl.registerService( serviceNameFoo, servicePidFoo );

    const std::string serviceNameBar = "ha-bar";
    const Watcher::Types::Pid servicePidBar = 2000;
    impl.registerService( serviceNameBar, servicePidBar );

    auto& heartbeatFoo = impl.m_heartbeatMap.at( serviceNameFoo );
    auto& heartbeatBar = impl.m_heartbeatMap.at( serviceNameBar );

    const std::chrono::milliseconds defaultTimeout( 20 );
    const std::chrono::milliseconds serviceBarTimeout( 50 );

    Watcher::IssueHandlers::UnresponsiveServicesHandler handler( std::move( implPtr ) );
    handler.setDefaultHeartbeatTimeout( defaultTimeout );
    handler.setPerProcessHeartbeatTimeout( { { serviceNameBar, serviceBarTimeout } } );

    // Timeout for foo and bar
    heartbeatFoo.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 150 ) );
    heartbeatBar.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 150 ) );
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet, std::set< Watcher::Types::Pid >( { servicePidFoo, servicePidBar } ) );
    EXPECT_EQ( impl.m_restartedProcesses, std::set< std::string >( { serviceNameFoo, serviceNameBar } ) );
    impl.m_restartedProcesses.clear();

    // Disable timeout for specific service bar
    handler.setPerProcessHeartbeatTimeout( { { serviceNameBar, std::chrono::milliseconds( 0 ) } } );
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet, std::set< Watcher::Types::Pid >( { servicePidFoo } ) );
    EXPECT_EQ( impl.m_restartedProcesses, std::set< std::string >( { serviceNameFoo } ) );
    impl.m_restartedProcesses.clear();

    // Disable timeout for default and specific services
    handler.setDefaultHeartbeatTimeout( std::chrono::milliseconds( 0 ) );
    handler.setPerProcessHeartbeatTimeout( { { serviceNameBar, std::chrono::milliseconds( 0 ) } } );
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet.size(), 0 );
    EXPECT_EQ( impl.m_restartedProcesses.size(), 0 );

    // Disable default timeout for foo but keep specific timeout for bar
    handler.setDefaultHeartbeatTimeout( std::chrono::milliseconds( 0 ) );
    handler.setPerProcessHeartbeatTimeout( { { serviceNameBar, std::chrono::milliseconds( 150 ) } } );
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet, std::set< Watcher::Types::Pid >( { servicePidBar } ) );
    EXPECT_EQ( impl.m_restartedProcesses, std::set< std::string >( { serviceNameBar } ) );
    impl.m_restartedProcesses.clear();
}

TEST_F( UnresponsiveServicesHandlerTestSuite, doNotRestartStoppedServices ) /* 1.1 */ {
    auto implPtr = std::make_unique< FakeImpl >();
    auto& impl = *implPtr;

    const std::string serviceNameFoo = "ha-foo";
    const Watcher::Types::Pid servicePidFoo = 1000;
    impl.registerService( serviceNameFoo, servicePidFoo );

    const std::string serviceNameBar = "ha-bar";
    const Watcher::Types::Pid servicePidBar = 2000;
    impl.registerService( serviceNameBar, servicePidBar );

    auto& heartbeatFoo = impl.m_heartbeatMap.at( serviceNameFoo );
    auto& heartbeatBar = impl.m_heartbeatMap.at( serviceNameBar );

    const std::chrono::milliseconds defaultTimeout( 20 );
    const std::chrono::milliseconds serviceBarTimeout( 50 );

    Watcher::IssueHandlers::UnresponsiveServicesHandler handler( std::move( implPtr ) );
    handler.setDefaultHeartbeatTimeout( defaultTimeout );
    handler.setPerProcessHeartbeatTimeout( { { serviceNameBar, serviceBarTimeout } } );

    // Timeout for foo and bar
    heartbeatFoo.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 150 ) );
    heartbeatBar.lastUpdateTime = getTimepointInPast( std::chrono::milliseconds( 150 ) );
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet, std::set< Watcher::Types::Pid >( { servicePidFoo, servicePidBar } ) );
    EXPECT_EQ( impl.m_restartedProcesses, std::set< std::string >( { serviceNameFoo, serviceNameBar } ) );
    impl.m_restartedProcesses.clear();

    // Change pid numbers
    for( auto& info : impl.m_processInfoList )
        info.pid = info.pid + 120;

    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_unresponsiveProcessesSet.size(), 0 );
    EXPECT_EQ( impl.m_restartedProcesses.size(), 0 );
    impl.m_restartedProcesses.clear();
}

}  // namespace UT
}  // namespace HubPlatform
