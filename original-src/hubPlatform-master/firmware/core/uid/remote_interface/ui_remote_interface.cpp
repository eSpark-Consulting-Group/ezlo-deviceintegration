#include "uid/remote_interface/ui_remote_interface.h"
#include "uid/remote_interface/http_server_api.h"
#include "uid/common_methods.h"

#include "service_interfaces/service.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"
#include <cassert>

namespace HubPlatform {
namespace Uid {

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService )
, m_httpServerApiUPtr( std::make_unique< HttpServerApi >( m_hostService ).release() ) {}

RemoteInterface::~RemoteInterface() = default;

bool RemoteInterface::isRegisteredOnCloud() const {
    auto result = m_hostService.getDBClient().findOne(
        HaTempCacheDBName, JSON( db::ID_FIELD_NAME << "cache-cloud-status" ), nullptr );
    return json_get_bool( result, "registered" );
}

JsonObject RemoteInterface::getHubInfo() const {
    return Common::getHubInfo( m_hostService );
}

HttpServerRemoteInterface& RemoteInterface::httpServer() {
    assert( m_httpServerApiUPtr != nullptr );
    return *m_httpServerApiUPtr;
}

JsonObject RemoteInterface::fetchLocalServerPorts() const {
    Json fields = JSON( "securePort" << 1 << "insecurePort" << 1 );
    return m_hostService.getDBClient().findOne(
        HaTempCacheDBName, JSON( db::ID_FIELD_NAME << "local_server_started" ), &fields );
}

}  // namespace Uid
}  // namespace HubPlatform
