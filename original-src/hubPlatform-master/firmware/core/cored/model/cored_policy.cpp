#include "cored/model/cored_policy.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

using namespace Resources::PolicyFields;

Policy::Policy( const std::string& name,
                const std::string& m_protectConfig,
                const std::string& deviceGroup,
                const JsonArray meshbots,
                bool persistent )
: m_name( name )
, m_protectConfig( m_protectConfig )
, m_deviceGroup( deviceGroup )
, m_meshbots( std::move( meshbots ) )
, m_persistent( persistent ) {}

Json Policy::toJson() const {
    auto policy = JSON_EMPTY_OBJ;

    policy[ Name ] = m_name;
    policy[ ProtectConfig ] = m_protectConfig;
    policy[ DeviceGroup ] = m_deviceGroup;
    policy[ MeshBots ] = m_meshbots;
    policy[ Persistent ] = m_persistent;

    return policy;
}

void Policy::checkPolicyAsJson( const Json& policy, bool isGetDeviceListCall ) {
    jsonCheckString( policy, Name );
    jsonCheckBool( policy, Persistent );

    if( json_contains( policy, ProtectConfig ) ) {
        jsonCheckString( policy, ProtectConfig );
    }
    if( json_contains( policy, DeviceGroup ) ) {
        jsonCheckString( policy, DeviceGroup );
    }
    if( json_contains( policy, MeshBots ) ) {
        if( json_get_str( policy, MeshBots ).length() != 0 ) {
            jsonCheckStringArray( policy, MeshBots, true );
        }
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
