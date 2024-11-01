#include "update_manager/remote_interface/um_remote_interface.h"
#include "update_manager/resources/um_resources.h"

#include "database_client/dbqueryparser.h"
#include "service_interfaces/service.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace UpdateManager {

namespace {
constexpr const std::chrono::seconds InitializationCheckPeriod( 1 );
constexpr const int StallChecks = 10;
}  // namespace

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

CommonUtils::HubState RemoteInterface::requestSoftwareInfo() {
    auto result = m_hostService.sendRequest( GroupsNames::UPDATE_MANAGER, SOFTWARE_INFO_REQUEST, JSON_EMPTY_OBJ );
    return CommonUtils::HubState::fromJson( result );
}

void RemoteInterface::requestUpdatePlan( UpdateOperation operation, const std::string& operationId ) {
    auto params = JSON_EMPTY_OBJ;
    params[ "operation" ] = operation.toJson();
    params[ "operationId" ] = operationId;
    m_hostService.sendMethodCall( GroupsNames::UPDATE_MANAGER, SOFTWARE_UPDATE_PLAN_REQUEST, params );
}

void RemoteInterface::executeUpdatePlan( UpdatePlan plan, const std::string& operationId ) {
    auto params = plan.toJson();
    params[ "operationId" ] = operationId;
    m_hostService.sendMethodCall( GroupsNames::UPDATE_MANAGER, SOFTWARE_UPDATE_PLAN_EXECUTE, params );
}

void RemoteInterface::executeHubReset( HubResetType resetType ) {
    auto params = hubResetTypeToJson( resetType );
    m_hostService.sendMethodCall( GroupsNames::UPDATE_MANAGER, HUB_RESET_REQUEST, params );
}

void RemoteInterface::executeLuupSwitch() {
    m_hostService.sendMethodCall( GroupsNames::UPDATE_MANAGER, LUUP_SWITCH, JSON_EMPTY_OBJ );
}

void RemoteInterface::executeBootloaderUpdate() {
    m_hostService.sendMethodCall( GroupsNames::UPDATE_MANAGER, BOOTLOADER_UPDATE, JSON_EMPTY_OBJ );
}

void RemoteInterface::executeRecoveryPartitionUpdate() {
    m_hostService.sendMethodCall( GroupsNames::UPDATE_MANAGER, RECOVERY_PARTITION_UPDATE, JSON_EMPTY_OBJ );
}

bool RemoteInterface::isPluginUnavailable( const std::string& pluginName ) const {
    int tries = 0;
    while( !m_hostService.getDBClient().has( Resources::DBTableNames::UnavailablePlugins,
                                             JSON( db::ID_FIELD_NAME << "UM.initialized" ) ) ) {
        if( ++tries >= StallChecks )
            return false;

        std::this_thread::sleep_for( InitializationCheckPeriod );
    }

    return m_hostService.getDBClient().has( Resources::DBTableNames::UnavailablePlugins,
                                            JSON( db::ID_FIELD_NAME << pluginName ) );
}

Json RemoteInterface::getCurrentUpdateState() const {
    auto record =
        m_hostService.getDBClient().findOne( HaUpdateManagerPendingUpdate,
                                             JSON( db::ID_FIELD_NAME << Resources::PendingUpdateFields::PendingUpdate ),
                                             nullptr );

    return record;
}

}  // namespace UpdateManager
}  // namespace HubPlatform
