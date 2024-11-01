#include "vidoo4hub/vidoo_client/vidoo_bus_client.h"

#include "vidoo4hub/vidoo4hub_exception.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Vidoo4Hub {

VidooBusClient::VidooBusClient( Services::Service& service, MessageCallback callback )
: m_service( service )
, m_callback( std::move( callback ) ) {
    if( !m_callback )
        throw Vidoo4HubError( "Invalid callback was passed to VidooBusClient" );
}

void VidooBusClient::setUp() {}

void VidooBusClient::tearDown() {}

void VidooBusClient::sendMessage( const JsonObject& msg ) {
    using namespace Vidoo4Hub::Resources;
    m_service.sendMethodCall( GroupsNames::VIDOO, VIDOO_SERVICE_MESSAGE, msg );
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
