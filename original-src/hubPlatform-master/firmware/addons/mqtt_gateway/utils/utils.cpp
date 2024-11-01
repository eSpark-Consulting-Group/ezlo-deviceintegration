#include "mqtt_gateway/utils/utils.h"

#include "mqtt_gateway/resources/resources.h"

#include "database_client/dbqueryparser.h"
#include "bus/messagesclasses.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace MQTTGateway {
namespace Utils {

Json buildErrorResponseBody( Errors::Error error, const std::string& reason ) {
    JsonObject responseBody;
    setError( responseBody, error, reason );
    return responseBody;
}

Json buildMQTTResponse( const std::string& group, const std::string& messageUUID, Json responseBody ) {
    namespace Fields = MQTTGateway::Resources::MessageFields;
    namespace Class = MQTTGateway::Resources::MQTTMessageClasses;

    Json result = { { Fields::UUID, std::move( messageUUID ) }, { Fields::ResponseBody, std::move( responseBody ) } };

    Json response;
    response[ Fields::MessageGroup ] = group;
    response[ Fields::MessageClass ] = Class::Response;
    response[ Fields::MessageSubclass ] =
        msgSubclassToStr(::MessageClasses::RESPONSE, ::MessageSubClasses::RESPONSE_RESULT );
    response[ Fields::Result ] = result;
    return response;
}

Json buildMQTTErrorResponse( const std::string& group,
                             const std::string& messageUUID,
                             Errors::Error error,
                             const std::string& reason ) {
    return buildMQTTResponse( group, messageUUID, buildErrorResponseBody( error, reason ) );
}

std::set< std::string > readClientsFromDB( IDbClient& dbClient ) {
    using namespace MQTTGateway::Resources::DB;
    auto result = dbClient.findAll( TableNames::RegisteredClientsDBName, JSON_EMPTY_OBJ, nullptr );

    std::set< std::string > clients;
    for( auto& client : result ) {
        clients.emplace( json_get_str( client, db::ID_FIELD_NAME ) );
    }
    return clients;
}

void addClientToDB( IDbClient& dbClient, const std::string& clientId ) {
    using namespace MQTTGateway::Resources::DB;
    dbClient.insert( TableNames::RegisteredClientsDBName, JSON( db::ID_FIELD_NAME << clientId ) );
}

void removeClientFromDB( IDbClient& dbClient, const std::string& clientId ) {
    using namespace MQTTGateway::Resources::DB;
    dbClient.remove( TableNames::RegisteredClientsDBName, JSON( db::ID_FIELD_NAME << clientId ) );
}

}  // namespace Utils
}  // namespace MQTTGateway
}  // namespace HubPlatform
