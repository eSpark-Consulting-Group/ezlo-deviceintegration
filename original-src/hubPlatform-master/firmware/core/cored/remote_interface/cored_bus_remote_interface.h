#pragma once

#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/remote_interface/bus_db_client.h"

namespace HubPlatform {
namespace CoreDRemoteInterface {

class CoreDBusRemoteInterface : public CoreDRemoteInterface {
public:
    explicit CoreDBusRemoteInterface( Services::Service& hostService );
    ~CoreDBusRemoteInterface() override;

private:
    IDbClient& getDBClient() override;
    const IDbClient& getDBClient() const override;

private:
    Services::Service& m_hostService;
    BusDbClient m_dbClient;
};

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
