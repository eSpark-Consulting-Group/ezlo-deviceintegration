#include "cored/model/cored_gateway_processes_utils.h"

#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

CoreD::Model::GatewayProcess getGatewayCurrentProcess( const IDbClient& dbClient, const std::string& gatewayId ) {
    JsonObject gatewayProcess =
        dbClient.findOne( TempCoreGatewayProcessesDBName, JSON( db::ID_FIELD_NAME << gatewayId ), nullptr );

    if( gatewayProcess.empty() ) {
        return CoreD::Model::GatewayProcess::Unknown;
    }

    auto processString = json_get_str( gatewayProcess, "process" );
    auto process = Model::gatewayProcessFromString( processString );
    return process;
}

bool isProcessRunning( const IDbClient& dbClient, CoreD::Model::GatewayProcess process ) {
    return dbClient.has( TempCoreGatewayProcessesDBName, JSON( "process" << Model::toString( process ) ) );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
