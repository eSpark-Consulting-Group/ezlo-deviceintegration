#include "hardware_manager/battery/battery_manager_factory.h"
#include "hardware_manager/utils/battery.h"

#if defined( __HUB_TYPE_H2_SECURE__ )
#include "hardware_manager/battery/bq27441g1/battery_manager.h"
#include "hardware_manager/battery/empirical_battery_manager/battery_manager.h"
#endif

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace HardwareManager {

std::unique_ptr< BatteryManager > HardwareManager::BatteryManagerFactory::createBatteryManager(
    Services::Service& hostService,
    Services::TaskLoop& taskLoop,
    IDbClient& dbClient ) {
#if defined( __HUB_TYPE_H2_SECURE__ )
    return Utils::hasBatteryFuelGaugeMeter() ?
               static_cast< std::unique_ptr< BatteryManager > >(
                   std::make_unique< Bq27441g1::BatteryManager >( taskLoop, dbClient ) ) :
               static_cast< std::unique_ptr< BatteryManager > >(
                   std::make_unique< EmpiricalBatteryManager::BatteryManager >( hostService ) );
#else
    return nullptr;
#endif
}

}  // namespace HardwareManager
}  // namespace HubPlatform
