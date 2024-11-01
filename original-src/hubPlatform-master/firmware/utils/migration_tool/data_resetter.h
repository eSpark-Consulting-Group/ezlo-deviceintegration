#pragma once

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace MigrationTool {

class DataResetter {
public:
    DataResetter( IDbClient& dbClient );

    bool resetData( bool soft = true );

private:
    IDbClient& m_dbClient;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
