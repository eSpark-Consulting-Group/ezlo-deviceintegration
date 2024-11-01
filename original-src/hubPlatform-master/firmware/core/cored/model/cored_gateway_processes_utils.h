#pragma once

#include "cored/model/cored_gateway_processes.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

CoreD::Model::GatewayProcess getGatewayCurrentProcess( const IDbClient& dbClient, const std::string& gatewayId );
bool isProcessRunning( const IDbClient& dbClient, CoreD::Model::GatewayProcess process );

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
