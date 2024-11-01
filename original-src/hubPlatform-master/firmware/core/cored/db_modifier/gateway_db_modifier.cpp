#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/model/resources/cored_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"

#include <stdexcept>

namespace HubPlatform {
namespace CoreD {

class DbModifierError : public std::runtime_error {
public:
    DbModifierError( const std::string& errDesription )
    : runtime_error( "DbModifier: " + errDesription ) {}
};

GatewayDbModifier::GatewayDbModifier( IDbClient& db, Broadcaster broadcaster )
: DbModifier( db,
              broadcaster,
              Model::Resources::DBNames::GatewaysDB,
              { { HUB_GATEWAY_ADDED, {} }, { HUB_GATEWAY_UPDATED, {} }, { HUB_GATEWAY_REMOVED, {} } },
              {} ) {}

GatewayDbModifier::~GatewayDbModifier() {}

}  // namespace CoreD
}  // namespace HubPlatform
