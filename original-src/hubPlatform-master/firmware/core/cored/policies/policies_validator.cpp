#include "policies_validator.h"

#include "resources.h"
#include "database_client/dbqueryparser.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/resources/cored_resources.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {
namespace PolicyValidator {

JsonObject validateCreatePolicyRequest( const JsonObject& params, bool checkRequiredParams ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    JsonPath path;
    auto gv = path.makeGuard( "params" );

    using namespace HubPlatform::CoreD::Resources;
    using namespace HubPlatform::CoreD;
    using namespace Model::Resources;

    try {
        jsonCheckString( params, PolicyFields::Name, checkRequiredParams );
        if( json_contains( params, PolicyFields::ProtectConfig ) ) {
            jsonCheckString( params, PolicyFields::ProtectConfig, checkRequiredParams );
        }
        if( json_contains( params, PolicyFields::DeviceGroup ) ) {
            jsonCheckString( params, PolicyFields::DeviceGroup, checkRequiredParams );
        }
        if( json_contains( params, PolicyFields::MeshBots ) ) {
            if( json_get_str( params, PolicyFields::MeshBots ).length() != 0 ) {
                jsonCheckStringArray( params, PolicyFields::MeshBots, checkRequiredParams );
            }
        }
    }
    catch( const WrongParamsError& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "", ex.what() );
        return answer;
    }
    catch( const WrongFieldError& ex ) {
        setErrorWithField( answer, Errors::WRONG_FIELD, "", ex.what() );
        return answer;
    }

    try {
        if( json_contains( params, PolicyFields::Persistent ) ) {
            jsonCheckBool( params, PolicyFields::Persistent, true );
        }
    }
    catch( const std::runtime_error& ex ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, ex.what(), PolicyFields::Persistent );
        return answer;
    }

    auto name = json_get_str( params, PolicyFields::Name );
    auto securityConfig = json_get_str( params, PolicyFields::ProtectConfig );
    auto deviceGroup = json_get_str( params, PolicyFields::DeviceGroup );
    try {
        jsonCheckString( name, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", PolicyFields::Name );
        return answer;
    }
    try {
        jsonCheckString( securityConfig, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", PolicyFields::ProtectConfig );
        return answer;
    }
    try {
        jsonCheckString( deviceGroup, JsonStringChecks::RESTRICTED_LENGTH );
    }
    catch( ... ) {
        setErrorWithField( answer, Errors::TOO_LONG, "", PolicyFields::DeviceGroup );
        return answer;
    }

    return answer;
}

}  // namespace PolicyValidator
}  // namespace CoreD
}  // namespace HubPlatform
