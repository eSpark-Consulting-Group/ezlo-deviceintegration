#pragma once

#include "service_interfaces/service.h"
#include "service_interfaces/tests/task_loop_test_helper.h"

#include "database_client/idbclient.h"
#include "database_client/fake_db/fake_db_client.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace Services {
namespace UT {

class ServiceFixture : public ::testing::Test {
public:
    virtual Service& getHostService() = 0;

public:
    void SetUp() override {
        cleanDB();
        m_taskLoopHelper.setTimeProviderForLoop( getHostService() );
        m_future = m_taskLoopHelper.runInThread( getHostService() );
    }

    void stopServiceAndWait() {
        getHostService().addTask( [&]() { getHostService().stop(); } );  // pop tasks for main-thread services
        m_future.get();
        m_taskLoopHelper.clearLoop( getHostService() );
    }

    void TearDown() override {
        stopServiceAndWait();
        cleanDB();
    }

    void cleanDB() {
        getHostService().getDBClient().clean();
    }

    void sleep( TaskLoop::Interval interval ) {
        m_taskLoopHelper.sleep( getHostService(), interval );
    }

    void waitForTasks() {
        m_taskLoopHelper.waitForTasks( getHostService() );
    }

private:
    TaskLoopTestHelper m_taskLoopHelper;
    std::future< void > m_future;
};

}  // namespace UT
}  // namespace Services
}  // namespace HubPlatform
