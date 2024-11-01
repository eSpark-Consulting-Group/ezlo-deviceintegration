#include "uid/remote_interface/http_server_api.h"
#include "uid/extensions/http_links/resources.h"
#include "service_interfaces/service.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Uid {

namespace {
std::string getErrorReason( const Json& answer ) {
    if( answer.count( "error" ) == 0 )
        return "";

    auto reason = json_get_str( answer[ "error" ], "reason" );
    if( reason.empty() )
        reason = json_get_str( answer[ "error" ], "message" );
    return reason;
}
}  // namespace

HttpServerApi::HttpServerApi( Services::Service& hostService )
: m_hostService( hostService ) {}

// http_server remote interface impl
Json HttpServerApi::createLinkUrl( std::string pluginId, Json params ) {
    params[ HttpLinks::Resources::HttpLinkFields::pluginId ] = pluginId;
    LOGGER_TRACE( cc::trace( " params: " ) + cc::j( params ) );
    auto answer =
        m_hostService.sendRequest( GroupsNames::UI_GROUP, params, MessageSubClasses::HTTP_LINKS_CREATE_LINK_URL );
    LOGGER_TRACE( cc::trace( " answer: " ) + cc::j( answer ) );
    if( hasError( answer ) ) {
        throw HttpServerError( getErrorReason( answer ) );
    }

    return answer;
}

bool HttpServerApi::deleteLinkUrl( std::string pluginId, std::string linkId ) {
    auto answer = m_hostService.sendRequest(
        GroupsNames::UI_GROUP,
        JSON( HttpLinks::Resources::HttpLinkFields::pluginId << pluginId << "linkId" << linkId ),
        MessageSubClasses::HTTP_LINKS_DELETE_LINK_URL );
    LOGGER_TRACE( cc::trace( " answer: " ) + cc::j( answer ) );
    if( hasError( answer ) ) {
        throw HttpServerError( getErrorReason( answer ) );
    }
    bool ok = !hasError( answer );
    return ok;
}

Json HttpServerApi::getLinkInfo( std::string pluginId, std::string linkId ) {
    auto answer = m_hostService.sendRequest(
        GroupsNames::UI_GROUP,
        JSON( HttpLinks::Resources::HttpLinkFields::pluginId << pluginId << "linkId" << linkId ),
        MessageSubClasses::HTTP_LINKS_GET_LINK_INFO );
    LOGGER_TRACE( cc::trace( " answer: " ) + cc::j( answer ) );
    if( hasError( answer ) ) {
        throw HttpServerError( getErrorReason( answer ) );
    }
    return answer;
}

bool HttpServerApi::closeConnection( std::string pluginId, std::string connectionId ) {
    auto answer = m_hostService.sendRequest(
        GroupsNames::UI_GROUP,
        JSON( HttpLinks::Resources::HttpLinkFields::pluginId
              << pluginId << HttpLinks::Resources::HttpLinkFields::connectionId << connectionId ),
        MessageSubClasses::HTTP_LINKS_CLOSE_CONNECTION );
    LOGGER_TRACE( cc::trace( " answer: " ) + cc::j( answer ) );
    if( hasError( answer ) ) {
        throw HttpServerError( getErrorReason( answer ) );
    }
    bool ok = !hasError( answer );
    return ok;
}

void HttpServerApi::sendData( std::string pluginId, Json params ) {
    params[ HttpLinks::Resources::HttpLinkFields::pluginId ] = pluginId;

    auto answer = m_hostService.sendRequest( GroupsNames::UI_GROUP, params, MessageSubClasses::HTTP_LINKS_SEND_DATA );
    LOGGER_TRACE( cc::trace( " answer: " ) + cc::j( answer ) );
    if( hasError( answer ) ) {
        throw HttpServerError( getErrorReason( answer ) );
    }
}

Json HttpServerApi::getConnectionInfo( std::string pluginId, std::string connectionId ) {
    using namespace HttpLinks::Resources;
    auto answer = m_hostService.sendRequest(
        GroupsNames::UI_GROUP,
        JSON( HttpLinks::Resources::HttpLinkFields::pluginId
              << pluginId << HttpLinks::Resources::HttpLinkFields::connectionId << connectionId ),
        MessageSubClasses::HTTP_LINKS_GET_CONNECTION_INFO );
    LOGGER_TRACE( cc::trace( " answer: " ) + cc::j( answer ) );
    if( hasError( answer ) ) {
        throw HttpServerError( getErrorReason( answer ) );
    }
    return answer;
}

}  // namespace Uid
}  // namespace HubPlatform
