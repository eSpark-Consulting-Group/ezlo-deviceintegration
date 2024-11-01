#pragma once

#include "hardware_manager/battery/battery_manager.h"

#include "service_interfaces/task_loop.h"
#include "service_interfaces/service.h"
#include "database_client/idbclient.h"

#include <memory>

namespace HubPlatform {
namespace HardwareManager {

class BatteryManagerFactory {
public:
    static std::unique_ptr< BatteryManager > createBatteryManager( Services::Service& hostService,
                                                                   Services::TaskLoop& taskLoop,
                                                                   IDbClient& dbClient );
};

}  // namespace HardwareManager
}  // namespace HubPlatform
