#include "cored/db_modifier/policy_db_modifier.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace CoreD {

PolicyDbModifier::PolicyDbModifier( IDbClient& db, HubPlatform::CoreD::DbModifier::Broadcaster broadcaster )
: DbModifier( db,
              broadcaster,
              Model::Resources::DBNames::PoliciesDB,
              { { POLICY_CREATED, {} }, { POLICY_UPDATED, {} }, { POLICY_DELETED, {} } },
              {} ) {}

}  // namespace CoreD
}  // namespace HubPlatform
