#include "watcher/issues_monitor/handlers/uninterruptible_sleep_handler.h"

#include "watcher/tests/fixture/database_fixture.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"
#include "service_metrics_data/hub_errors_table/hub_errors_table.h"

#include <gtest/gtest.h>

#include <future>

/*

TESTPLAN

DONE    1. Uninterruptible sleep tests
DONE        1.1 Recovery with initial 'D' state
DONE        1.2 Recovery without initial 'D' state

*/

namespace HubPlatform {
namespace UT {

class UninterruptibleSleepHandlerTestSuite : public Watcher::UT::DatabaseFixture {
public:
    using ProcessInfoList = Watcher::IssueHandlers::UninterruptibleSleepHandler::ProcessInfoList;

    class FakeImpl : public HubPlatform::Watcher::IssueHandlers::UninterruptibleSleepHandler::Impl {
    public:
        ProcessInfoList readProcessInfoList() override {
            ++m_readsCount;
            return m_processInfoList;
        }

        void runRecoveryAction() override {
            ++m_actionsCount;
        }

        ProcessInfoList m_processInfoList;
        size_t m_readsCount = 0;
        size_t m_actionsCount = 0;
    };

    ProcessInfoList getProcessListWithDStates() {
        ProcessInfoList list( 2 );
        {
            auto& info = list.at( 0 );
            info.pid = 7;
            info.state = "DW";
            info.cmd = "ha-networkd";
        }

        {
            auto& info = list.at( 1 );
            info.pid = 11147;
            info.state = "D";
            info.cmd = "ip route";
        }

        return list;
    }

    ProcessInfoList getProcessListWithoutDStates() {
        ProcessInfoList list( 2 );
        {
            auto& info = list.at( 0 );
            info.pid = 7;
            info.state = "SW";
            info.cmd = "ha-networkd";
        }

        {
            auto& info = list.at( 1 );
            info.pid = 11147;
            info.state = "S";
            info.cmd = "ip route";
        }

        return list;
    }
};

TEST_F( UninterruptibleSleepHandlerTestSuite, recoveryWithInitialDStates ) /* 1.1 */ {
    const auto UninterruptibleSleepError = ServiceMetricsData::HubErrorsTable::ErrorType::UninterruptibleSleep;
    ServiceMetricsData::HubErrorsTable errorTable( getDBClient() );

    auto implPtr = std::make_unique< FakeImpl >();

    auto& impl = *implPtr;
    impl.m_processInfoList = getProcessListWithDStates();

    const std::chrono::milliseconds timeout( 5 );

    Watcher::IssueHandlers::UninterruptibleSleepHandler handler( getDBClient(), std::move( implPtr ) );
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 0 );

    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 1 );
    EXPECT_EQ( impl.m_actionsCount, 0 ) << "Don't trip on first detection";
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 0 );

    handler.setUninterruptibleSleepTimeout( timeout );
    std::this_thread::sleep_for( timeout );

    // trip
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 2 );
    EXPECT_EQ( impl.m_actionsCount, 1 );
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 1 );

    // next trip
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 3 );
    EXPECT_EQ( impl.m_actionsCount, 2 );
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 1 ) << "Should be reported once";

    impl.m_processInfoList = getProcessListWithoutDStates();
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 4 );
    EXPECT_EQ( impl.m_actionsCount, 2 ) << "Should be reset";
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 1 );

    impl.m_processInfoList = getProcessListWithDStates();
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 5 );
    EXPECT_EQ( impl.m_actionsCount, 2 ) << "Don't trip on first detection";
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 1 );

    std::this_thread::sleep_for( timeout );

    // trip
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 6 );
    EXPECT_EQ( impl.m_actionsCount, 3 );
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 2 );
}

TEST_F( UninterruptibleSleepHandlerTestSuite, recoveryWithoutInitialDStates ) /* 1.2 */ {
    const auto UninterruptibleSleepError = ServiceMetricsData::HubErrorsTable::ErrorType::UninterruptibleSleep;
    ServiceMetricsData::HubErrorsTable errorTable( getDBClient() );

    auto implPtr = std::make_unique< FakeImpl >();
    auto& impl = *implPtr;

    impl.m_processInfoList = getProcessListWithoutDStates();

    const std::chrono::milliseconds timeout( 5 );

    Watcher::IssueHandlers::UninterruptibleSleepHandler handler( getDBClient(), std::move( implPtr ) );
    handler.setUninterruptibleSleepTimeout( timeout );
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 0 );

    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 1 );
    EXPECT_EQ( impl.m_actionsCount, 0 );
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 0 );

    std::this_thread::sleep_for( timeout );

    // no trip
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 2 );
    EXPECT_EQ( impl.m_actionsCount, 0 );
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 0 );

    // introduce 'D' states
    impl.m_processInfoList = getProcessListWithDStates();
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 3 );
    EXPECT_EQ( impl.m_actionsCount, 0 ) << "Don't trip on first detection";
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 0 );

    std::this_thread::sleep_for( timeout );

    // trip
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 4 );
    EXPECT_EQ( impl.m_actionsCount, 1 );
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 1 );

    impl.m_processInfoList = getProcessListWithoutDStates();
    EXPECT_NO_THROW( handler.run() );
    EXPECT_EQ( impl.m_readsCount, 5 );
    EXPECT_EQ( impl.m_actionsCount, 1 ) << "Should be reset";
    EXPECT_EQ( errorTable.getError( UninterruptibleSleepError ).count, 1 );
}

}  // namespace UT
}  // namespace HubPlatform
