#pragma once

#include "test_utils/temporary_root_folder_wrapper.h"
#include "database_client/idbclient.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>

namespace HubPlatform {
namespace UT {

class MigrationTestFixture : public testing::Test {
public:
    void SetUp() override {
        m_dbClient = createDbClient();
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
    Tests::TemporaryRootFolderWrapper m_rootFolderWrapper;
    std::unique_ptr< IDbClient > m_dbClient;
};

}  // namespace UT
}  // namespace HubPlatform