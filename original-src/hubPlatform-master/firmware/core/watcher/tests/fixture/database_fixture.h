#pragma once

#include "database_client/idbclient.h"
#include "database_client/fake_db/fake_db_client.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

#include <memory>

namespace HubPlatform {
namespace Watcher {
namespace UT {

class DatabaseFixture : public testing::Test {
public:
    void SetUp() override {
        m_dbClient = std::make_unique< HubPlatform::UT::FakeDbClient >();
        m_dbClient->clean();
    }

    void TearDown() override {
        m_dbClient->clean();
        m_dbClient.reset();
    }

    IDbClient& getDBClient() {
        assert( m_dbClient );
        return *m_dbClient;
    }

private:
    std::unique_ptr< IDbClient > m_dbClient;
};

}  // namespace UT
}  // namespace Watcher
}  // namespace HubPlatform
