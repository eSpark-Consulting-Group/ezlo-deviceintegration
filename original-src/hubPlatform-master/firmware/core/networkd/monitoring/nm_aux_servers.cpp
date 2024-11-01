#include "nm_aux_servers.h"
#include <mutex>
#include "log_utils/ezlo_log.h"
#include "networkd/resources/nm_resources.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace Network {

std::mutex& getPingEnvMutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector< std::string > g_additionalServers = {};
std::chrono::seconds g_pingTimeout{ 10 };

void setAdditionalServers( std::vector< std::string > servers ) {
    std::lock_guard< std::mutex > lock( getPingEnvMutex() );
    g_additionalServers = std::move( servers );
}

std::vector< std::string > getAdditionalServers() {
    std::lock_guard< std::mutex > lock( getPingEnvMutex() );
    return g_additionalServers;
}

void setPingTimeout( std::chrono::seconds timeout ) {
    std::lock_guard< std::mutex > lock( getPingEnvMutex() );
    LOGGER_INFO_RAW( cc::info( "Set ping timeout to " ) + cc::num10( timeout.count() ) + cc::info( " seconds" ) );
    g_pingTimeout = timeout;
}

std::chrono::seconds getPingTimeout() {
    std::lock_guard< std::mutex > lock( getPingEnvMutex() );
    return g_pingTimeout;
}

/** Returns object, where keys are cloud server roles, and value is an object:
 *      { server: domainName, fullUrl: httpOrWsEndpoint } */
Json& namedAuxServers() {
    using namespace Resources::ServerRoles;
    using namespace Resources::AuxServerFields;
    static Json auxServers = { { NmaServer, { { Server, "" }, { FullUrl, "" } } },
                               { CloudCoordinator, { { Server, "" }, { FullUrl, "" } } } };

    return auxServers;
}

void registerAdditionalServers( const std::string& role, const std::string& server, const std::string& fullUrl ) {
    using namespace Resources::ServerRoles;
    using namespace Resources::AuxServerFields;
    // Save nma and cloud coordinator server here
    if( role == NmaServer || role == CloudCoordinator ) {
        namedAuxServers()[ role ] = JSON( Server << server << FullUrl << fullUrl );
    }
}

}  // namespace Network
}  // namespace HubPlatform
