#include "policies_operations_handler.h"
#include "cored/policies/policies_validator.h"
#include "cored/policies/policies_updater.h"
#include "cored/policies/resources.h"
#include "cored/policies/policies.h"
#include "cored/model/resources/cored_resources.h"
#include "crypto_utils/digests.h"
#include "base_utils/ezlodb.h"
#include "common_utils/cpp14.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "jsonrpc/jsonrpc.h"
#include <unistd.h>

namespace HubPlatform {
namespace CoreD {
using namespace Resources;

static constexpr auto fieldID = "_id";
static constexpr auto requestID = "id";

IDbClient& PolicyOperationsHandler::getDBClient() {
    return m_dbClient;
}

PolicyOperationsHandler::PolicyOperationsHandler( IDbClient& dbClient,
                                                  BroadcastCallback& sendBroadcast,
                                                  CoreDRemoteInterface::CoreDInterface& core )
: m_dbClient( dbClient )
, m_sendBroadcast( sendBroadcast )
, m_core( core ) {}

JsonObject PolicyOperationsHandler::validatePolicyRequest( const JsonObject& message, bool checkRequiredParams ) {
    return PolicyValidator::validateCreatePolicyRequest( message, checkRequiredParams );
}

JsonObject PolicyOperationsHandler::processPolicyCreateRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;
    using namespace Policies;

    LOGGER_INFO( "Create policy: " + cc::j( params ) );

    Json answer;

    answer = validatePolicyRequest( params, true );
    if( !answer.empty() ) {
        LOGGER_ERROR( cc::error( "Policy validation failed: " ) + cc::j( answer ) );
        return answer;
    }

    std::string id;
    if( json_contains( params, requestID ) ) {
        id = json_get_str( params, requestID );
        if( id.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            LOGGER_ERROR( cc::error( "Policy ID is empty: " ) + cc::j( answer ) );
            return answer;
        }
    }
    else {
        id = Db::genOid();
    }

    auto policyId = JSON( fieldID << id );
    JsonObject dbItem = getDBClient().findOne( DBNames::PoliciesDB, policyId, nullptr );
    if( dbItem.empty() ) {
        PolicyInfo info;
        info.fromJson( params );
        info.setId( id );
        auto requestParams = info.toJson();
        getDBClient().insert( DBNames::PoliciesDB, requestParams );
        if( m_sendBroadcast != nullptr ) {
            m_sendBroadcast( POLICY_CREATED, requestParams );
        }
    }
    else {
        setErrorWithField( answer, Errors::SUCH_NAME_ALREADY_EXISTS, "The policy with the same name is exist", "" );
        LOGGER_ERROR( cc::error( "Policy name already exists: " ) + cc::j( answer ) );
    }

    return answer;
}

JsonObject PolicyOperationsHandler::processPolicyDeleteRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;
    using namespace Policies;

    LOGGER_INFO( "Delete policy: " + cc::j( params ) );

    Json answer;

    JsonObject idObject;
    if( json_get( params, requestID, idObject ) ) {
        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            LOGGER_ERROR( cc::error( "Policy ID is empty: " ) + cc::j( answer ) );
            return answer;
        }

        auto id = idObject.get< std::string >();
        auto policyId = JSON( fieldID << id );
        JsonObject dbItem = getDBClient().findOne( DBNames::PoliciesDB, policyId, nullptr );
        if( dbItem.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Record with the key does not exist", requestID );
            LOGGER_ERROR( cc::error( "Policy with that ID does not exist: " ) + cc::j( answer ) );
        }
        else {
            PolicyInfo info;
            info.fromJson( dbItem );

            if( !info.getPersistent() ) {
                getDBClient().remove( DBNames::PoliciesDB, policyId );
                if( m_sendBroadcast != nullptr ) {
                    m_sendBroadcast( POLICY_DELETED, policyId );
                }
            }
            else {
                setErrorWithField( answer, Errors::DELETE_GROUP_IMPOSSIBLE, "Can't delete persistent policy", "" );
                LOGGER_ERROR( cc::error( "Persistent policies can not be deleted: " ) + cc::j( answer ) );
            }
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", requestID );
        LOGGER_ERROR( cc::error( "Policy ID is not present: " ) + cc::j( answer ) );
    }

    return answer;
}

JsonObject PolicyOperationsHandler::processPolicyUpdateRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;
    using namespace Policies;

    LOGGER_INFO( "Update policy: " + cc::j( params ) );

    Json answer;

    JsonObject idObject;
    if( json_get( params, requestID, idObject ) ) {
        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            LOGGER_ERROR( cc::error( "Policy ID is empty: " ) + cc::j( answer ) );
            return answer;
        }

        auto id = idObject.get< std::string >();
        auto policyId = JSON( fieldID << id );
        JsonObject dbItem = getDBClient().findOne( DBNames::PoliciesDB, policyId, nullptr );
        if( dbItem.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Record with the key does not exist", requestID );
            LOGGER_ERROR( cc::error( "Policy with that ID does not exist: " ) + cc::j( answer ) );
        }
        else {
            answer = validatePolicyRequest( params, false );
            if( answer.empty() ) {
                PolicyInfo info;
                info.fromJson( dbItem );
                auto savedPolicy = info.toJson();
                Json updatedJson = PolicyUpdater::getPolicyUpdateData( savedPolicy, params );
                getDBClient().update( DBNames::PoliciesDB, JSON( fieldID << id ), JSON( "$set" << updatedJson ) );
                auto updatedObject = processPolicyUpdateBroadcastData( params, savedPolicy );
                if( !updatedObject.empty() && m_sendBroadcast != nullptr ) {
                    updatedObject[ fieldID ] = id;

                    if( json_get_bool( updatedObject, PolicyFields::Sync ) == false ) {
                        updatedObject[ PolicyFields::Name ] = info.getName();
                    }

                    m_sendBroadcast( POLICY_UPDATED, updatedObject );
                }
            }
            else {
                LOGGER_ERROR( cc::error( "Policy validation failed: " ) + cc::j( answer ) );
                return answer;
            }
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", requestID );
        LOGGER_ERROR( cc::error( "Policy ID is not present: " ) + cc::j( answer ) );
    }

    return answer;
}

JsonObject PolicyOperationsHandler::processPolicyGetRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;
    using namespace Policies;

    LOGGER_INFO( "Get policy: " + cc::j( params ) );

    Json answer;

    JsonObject idObject;
    if( json_get( params, requestID, idObject ) ) {
        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            LOGGER_ERROR( cc::error( "Policy ID is empty: " ) + cc::j( answer ) );
            return answer;
        }

        auto id = idObject.get< std::string >();
        auto policyId = JSON( fieldID << id );
        JsonObject dbItem = getDBClient().findOne( DBNames::PoliciesDB, policyId, nullptr );
        if( dbItem.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Record with the key does not exist", requestID );
            LOGGER_ERROR( cc::error( "Policy with that ID does not exist: " ) + cc::j( answer ) );
        }
        else {
            PolicyInfo info;
            info.fromJson( dbItem );
            answer = info.toJson();
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", requestID );
        LOGGER_ERROR( cc::error( "Policy ID is not present: " ) + cc::j( answer ) );
    }

    return answer;
}

JsonObject PolicyOperationsHandler::processPolicyListRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;
    using namespace Policies;

    LOGGER_INFO( "List policies: " + cc::j( params ) );

    Json answer;
    PolicyInfo policyInfo;

    std::set< std::string > ids;
    JsonArray strings;
    JsonObject stringsObj;
    if( json_get( params, "policyIds", strings ) ) {
        if( !strings.is_array() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", "policyIds" );
            LOGGER_ERROR( cc::error( "Policy IDs need to be an array: " ) + cc::j( answer ) );
            return answer;
        }
        try {
            fromJsonStringArray( strings, [&ids]( const std::string& value ) { ids.emplace( value ); } );
        }
        catch( std::exception& ex ) {
            std::string reason = "Wrong value type: ";
            reason += ex.what();
            setErrorWithField( answer, Errors::WRONG_PARAMS, reason, "policyIds" );
            LOGGER_ERROR( cc::error( "Policy IDs need to be strings: " ) + cc::j( answer ) );
            return answer;
        }
    }

    std::string versionParams;
    Json version = params[ "version" ];
    if( !version.empty() ) {
        if( !version.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", "version" );
            LOGGER_ERROR( cc::error( "Policy version need to be a string: " ) + cc::j( answer ) );
            return answer;
        }
        versionParams = json_get_str( params, "version" );
    }

    auto entries = getDBClient().findAll( DBNames::PoliciesDB, JSON_EMPTY_OBJ, nullptr );
    JsonArrayBuilder filteredPolicies;
    std::stringstream ss;

    auto processParams = [&filteredPolicies, &ss]( const JsonObject& entry ) {
        filteredPolicies << entry;
        ss << entry;
    };

    for( const auto& entry : entries ) {
        policyInfo.fromJson( entry );
        auto id = json_get_str( entry, fieldID );
        if( !ids.empty() && ids.count( id ) ) {
            processParams( policyInfo.toJson() );
        }
        else if( ids.empty() ) {
            processParams( policyInfo.toJson() );
        }
    }

    ss << getpid();

    auto hash = CryptoUtils::Digests::sha1( ss );

    if( !version.empty() && hash == versionParams ) {
        return JSON( "version" << hash );
    }

    answer = JSON( "version" << hash << "policies" << filteredPolicies );

    return answer;
}

JsonObject PolicyOperationsHandler::processPolicyUpdateBroadcastData( const JsonObject& updatedData,
                                                                      const JsonObject& savedData ) {
    return PolicyUpdater::getPolicyUpdateBroadcastData( updatedData, savedData );
}

}  // namespace CoreD
}  // namespace HubPlatform
