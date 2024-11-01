#include "watcher/issues_monitor/handlers/wifi_bcdc_error_handler.h"
#include "watcher/resources/watcher_resources.h"
#include "watcher/tests/fakes/fake_system_log.h"
#include "watcher/tests/fixture/database_fixture.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"
#include "service_metrics_data/hub_errors_table/hub_errors_table.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. WifiBcdcErrorHandler tests
DONE        1.1 Detect WiFi BCDC error

*/

namespace HubPlatform {
namespace UT {

class WifiBcdcErrorHandlerTestSuite : public Watcher::UT::DatabaseFixture {
public:
    class FakeImpl : public HubPlatform::Watcher::IssueHandlers::WifiBcdcErrorHandler::Impl {
    public:
        void runRecoveryAction() override {
            ++m_actionsCount;
        }
        size_t m_actionsCount = 0;
    };
};

TEST_F( WifiBcdcErrorHandlerTestSuite, detectWifiBcdcError ) /* 1.1 */ {
    const auto BCDCError = ServiceMetricsData::HubErrorsTable::ErrorType::BCDCError;
    ServiceMetricsData::HubErrorsTable errorTable( getDBClient() );

    FakeSystemLog systemLog;
    auto implPtr = std::make_unique< FakeImpl >();

    auto& impl = *implPtr;

    Watcher::IssueHandlers::WifiBcdcErrorHandler handler( getDBClient(), systemLog, std::move( implPtr ) );
    EXPECT_EQ( errorTable.getError( BCDCError ).count, 0 );

    ASSERT_EQ( systemLog.m_subscribers.size(), 1 );
    EXPECT_EQ( impl.m_actionsCount, 0 );

    handler.run();
    EXPECT_EQ( impl.m_actionsCount, 0 );
    EXPECT_EQ( errorTable.getError( BCDCError ).count, 0 );

    systemLog.publishLogRecord( "dummy log record" );
    handler.run();
    EXPECT_EQ( impl.m_actionsCount, 0 );

    std::string pattern =
        std::string( "prefix: " ) + Watcher::Resources::IssueHandlers::WifiBcdcErrorPattern + " :suffix";
    systemLog.publishLogRecord( pattern );
    handler.run();
    EXPECT_EQ( impl.m_actionsCount, 1 );
    EXPECT_EQ( errorTable.getError( BCDCError ).count, 1 );
    impl.m_actionsCount = 0;

    handler.run();
    EXPECT_EQ( impl.m_actionsCount, 0 ) << "Same event should not be triggered twice";
    EXPECT_EQ( errorTable.getError( BCDCError ).count, 1 );
}

}  // namespace UT
}  // namespace HubPlatform
