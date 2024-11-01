#include "cored/remote_interface/cored_bus_remote_interface.h"
#include "cored/remote_interface/bus_provision_data_manager.h"

namespace HubPlatform {
namespace CoreDRemoteInterface {

CoreDBusRemoteInterface::CoreDBusRemoteInterface( Services::Service& hostService )
: CoreDRemoteInterface( hostService, getBusProvisionDataManager( hostService ).getProvisionData() )
, m_hostService( hostService )
, m_dbClient( hostService ) {}

CoreDBusRemoteInterface::~CoreDBusRemoteInterface() = default;

IDbClient& CoreDBusRemoteInterface::getDBClient() {
    return m_dbClient;
}

const IDbClient& CoreDBusRemoteInterface::getDBClient() const {
    return m_dbClient;
}

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
