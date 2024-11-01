#pragma once

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {

class BusDbManager final {
public:
    BusDbManager( IDbClient& dbClient );

    JsonObject handleRequestDBData( const JsonObject& params );
    void handlePerformDBOperation( const JsonObject& params );

private:
    IDbClient& m_dbClient;
};

}  // namespace CoreD
}  // namespace HubPlatform
