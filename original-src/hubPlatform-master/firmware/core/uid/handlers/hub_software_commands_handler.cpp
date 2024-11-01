#include <hardware_manager/battery/battery_info.h>
#include "uid/handlers/hub_software_commands_handler.h"
#include "uid/ui_service.h"
#include "uid/broadcast.h"

#include "update_manager/remote_interface/um_remote_interface.h"
#include "update_manager/resources/um_resources.h"
#include "common_utils/firmware_info_helper.h"
#include "hub_settings/hub_setting.h"
#include "cored/remote_interface/cored_interface_factory.h"

#include "base_utils/acl.h"

namespace HubPlatform {
namespace Uid {

HubSoftwareCommandsHandler::HubSoftwareCommandsHandler( Services::Service& hostService )
: m_updateManager( hostService )
, m_hardwareManager( hostService )
, m_core( CoreDRemoteInterface::makeInstance( hostService ) ) {}

void HubSoftwareCommandsHandler::invoke( const std::string& method, rpc::Message& rpcMessage ) {
    using RequestHandler = void ( HubSoftwareCommandsHandler::* )( rpc::Message & rpcMessage );
    using RequestHandlersMap = std::unordered_map< std::string, RequestHandler >;
    static const RequestHandlersMap requestHandlers = {
        { "info.get", &HubSoftwareCommandsHandler::handleInfoGetRequest },
        { "update.plan", &HubSoftwareCommandsHandler::handleUpdatePlanRequest },
        { "update.execute", &HubSoftwareCommandsHandler::handleUpdateExecuteRequest },
    };

    const auto it = requestHandlers.find( method );
    if( it != requestHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
        return;
    }

    JsonObject& joAnswer = rpcMessage.getAnswer();
    setError( joAnswer, Errors::UNKNOWN_METHOD );
}

void HubSoftwareCommandsHandler::handleInfoGetRequest( rpc::Message& rpcMessage ) {
    const JsonObject& message = rpcMessage.getMessage();

    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( rpcMessage.getParams(), Resource::EZLO, Permission::EDIT, joAnswer ) )
        return;

    auto hubState = m_updateManager.requestSoftwareInfo();

    JsonObject response = hubState.toJson();
    joAnswer[ "result" ] = std::move( response );
}

// params: {
//        "operation": "uninstall",
//        "source": "http://dl.mios.com/linux_firmware/qa",
//        "snapshot": 42,
//        "type": "plugin",
//        "id": "zwave",
//        "version": "1.2.42"
//    }
void HubSoftwareCommandsHandler::handleUpdatePlanRequest( rpc::Message& rpcMessage ) {
    using namespace UpdateManager;
    const JsonObject& message = rpcMessage.getMessage();
    auto params = rpcMessage.getParams();

    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( rpcMessage.getParams(), Resource::EZLO, Permission::EXECUTE, joAnswer ) )
        return;

    auto updateOperation = UpdateOperation::fromJson( params );
    if( !updateOperation.isValid() ) {
        setError( joAnswer, Errors::BAD_PARAMS );
        return;
    }

    if( !isUpdateEnabled() ) {
        Log::debug( cc::debug( "Update of firmware is prohibited" ) );
        JsonObject json;
        json[ "actions" ] = JSON_EMPTY_ARR;
        Broadcast::sendToUis( PLANNING_SOFTWARE_UPDATE, json );
        return;
    }

    // NOTE: return empty update plan if firmware is built locally
    if( FirmwareInfoHelper::getFirmwareVersion().getSuffix() == "0" ) {
        Log::debug( cc::debug( "We deny an update request for firmware with BUILD_VERSION = 0" ) );
        JsonObject json;
        json[ "actions" ] = JSON_EMPTY_ARR;
        Broadcast::sendToUis( PLANNING_SOFTWARE_UPDATE, json );
        return;
    }

    m_updateManager.requestUpdatePlan( updateOperation, rpcMessage.getId() );
}

// "params": {
//        "actions": [
//            {
//                "operation": "upgrade",
//                "source": "http://dl.mios.com/linux_firmware/qa",
//                "snapshot": 42,
//                "type": "firmware",
//                "version": "1.2.42"
//            }
//        ]
//    }
void HubSoftwareCommandsHandler::handleUpdateExecuteRequest( rpc::Message& rpcMessage ) {
    const JsonObject& message = rpcMessage.getMessage();
    auto params = rpcMessage.getParams();

    JsonObject& joAnswer = rpcMessage.getAnswer();
    if( !Acl::check( rpcMessage.getParams(), Resource::EZLO, Permission::EXECUTE, joAnswer ) )
        return;

    std::string errorMessage;
    std::string errorField;

    if( !UpdateManager::UpdatePlan::validateCustomPluginActions( params, errorMessage, errorField ) ) {
        setErrorWithField( joAnswer, Errors::BAD_PARAMS, errorMessage, errorField );
        return;
    }

    auto updatePlan = UpdateManager::UpdatePlan::fromJson( params );
    if( !updatePlan.isValid() ) {
        setErrorWithField( joAnswer, Errors::BAD_PARAMS, errorMessage, errorField );
        return;
    }

    auto battery = m_hardwareManager.getBatteryInfo();
    if( battery && ( ( battery->isDischarging() && battery->isLow() ) || battery->depleted ) ) {
        setError( joAnswer, Errors::BATTERY_LOW );
        return;
    }

    m_updateManager.executeUpdatePlan( updatePlan, rpcMessage.getId() );
}

bool HubSoftwareCommandsHandler::isUpdateEnabled() const {
    const auto setting = m_core->findSetting( "update.enable" );
    if( setting && setting->isValid() && setting->getValueType() == Hub::Setting::ValueType::Bool ) {
        return setting->getBoolValue();
    }

    return true;
}

}  // namespace Uid
}  // namespace HubPlatform
