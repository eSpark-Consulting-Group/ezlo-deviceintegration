#include "swinger_shutdown_interface.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

SwingerShutdownInterface::SwingerShutdownInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

JsonObject SwingerShutdownInterface::getSwingerShutdownList( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODES_SWINGER_SHUTDOWN_LIST );
}

JsonObject SwingerShutdownInterface::resetDeviceStatistics( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODES_SWINGER_SHUTDOWN_RESET );
}

JsonObject SwingerShutdownInterface::disableAdd( const JsonObject& params ) {
    return m_hostService.sendRequest(
        GroupsNames::CORED, params, MessageSubClasses::MODES_SWINGER_SHUTDOWN_DISABLE_ADD );
}

JsonObject SwingerShutdownInterface::disableRemove( const JsonObject& params ) {
    return m_hostService.sendRequest(
        GroupsNames::CORED, params, MessageSubClasses::MODES_SWINGER_SHUTDOWN_DISABLE_REMOVE );
}

JsonObject SwingerShutdownInterface::limitsSet( const JsonObject& params ) {
    return m_hostService.sendRequest(
        GroupsNames::CORED, params, MessageSubClasses::MODES_SWINGER_SHUTDOWN_LIMITS_SET );
}

JsonObject SwingerShutdownInterface::limitsReset( const JsonObject& params ) {
    return m_hostService.sendRequest(
        GroupsNames::CORED, params, MessageSubClasses::MODES_SWINGER_SHUTDOWN_LIMITS_RESET );
}

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform
