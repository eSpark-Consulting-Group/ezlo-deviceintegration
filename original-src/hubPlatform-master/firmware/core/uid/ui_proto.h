#pragma once

#include "uid/proto.h"
#include "uid/response_builders/getitems/iresponse_builder.h"

#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace Uid {

class UiProto : public Proto {
public:
    using Proto::Proto;

    void processBroadcast( const Services::Message& message ) override;

protected:
    bool invokeHandler( const std::string& method, rpc::Message& message ) override;
    bool invokeExtendedHandler( const std::string& method,
                                const std::string& submethod,
                                rpc::Message& message ) override;

private:
    void createRoom( rpc::Message& rpcMessage );
    void setRoomName( rpc::Message& rpcMessage );
    void deleteRoom( rpc::Message& rpcMessage );
    void deleteAllRooms( rpc::Message& rpcMessage );
    void getRoom( rpc::Message& rpcMessage );
    void setRoomsOrder( rpc::Message& rpcMessage );
    void getGateways( rpc::Message& rpcMessage );
    void listGatewaySettings( rpc::Message& rpcMessage );
    void setGatewaySettingValue( rpc::Message& rpcMessage );
    void getItemsByDeviceId( rpc::Message& rpcMessage );
    void renameDevice( rpc::Message& rpcMessage );
    void removeDeviceForce( rpc::Message& rpcMessage );
    void setDeviceArmed( rpc::Message& rpcMessage );
    void setDeviceProtectConfig( rpc::Message& rpcMessage );
    void placeDevice( rpc::Message& rpcMessage );
    void foSetFavorite( rpc::Message& rpcMessage );
    void foGetFavorites( rpc::Message& rpcMessage );
    void getWhatHappened( rpc::Message& rpcMessage );
    void setPreferences( rpc::Message& rpcMessage );
    void getPreferences( rpc::Message& rpcMessage );
    void rebootEzlo( rpc::Message& rpcMessage );
    void getPowerStatus( rpc::Message& rpcMessage );
    void hubReset( rpc::Message& rpcMessage );
    void hubBootloaderUpdate( rpc::Message& rpcMessage );
    void hubRecoveryPartitionUpdate( rpc::Message& rpcMessage );

    void justIgnoreMethod( rpc::Message& rpcMessage );

    void getHubStatus( rpc::Message& rpcMessage );
    void getTemplateForDevice( rpc::Message& rpcMessage );

    void setHubCoords( rpc::Message& rpcMessage );
    void getHubCoords( rpc::Message& rpcMessage );
    void setHubTimeZone( rpc::Message& rpcMessage );
    void getHubTimeZone( rpc::Message& rpcMessage );
    void listHubTimeZones( rpc::Message& rpcMessage );

    void addItemDictionaryValue( rpc::Message& rpcMessage );
    void setItemDictionaryValue( rpc::Message& rpcMessage );
    void removeItemDictionaryValue( rpc::Message& rpcMessage );
    void clearItemDictionary( rpc::Message& rpcMessage );

    void invokeExtensionsHandler( const std::string& method, rpc::Message& rpcMessage );
    void invokeHubFirmwareHandler( const std::string& method, rpc::Message& rpcMessage );  // NOTE: legacy, do not use
    void invokeHubSshTunnelHandler( const std::string& method, rpc::Message& rpcMessage );
    void invokeDeviceAddingWizardHandler( const std::string& deviceWizardMethod, rpc::Message& rpcMessage );
    void invokeScenesHandler( const std::string& scenesMethod, rpc::Message& rpcMessage );
    bool invokeHubHandler( const std::string& method, rpc::Message& rpcMessage );

    void invokeHouseModeHandler( const std::string& houseModeMethod, rpc::Message& rpcMessage );

    std::string getCurrentHouseModeImpl();
    std::string getHouseModeSwitchToImpl();

    void invokeHouseModeGetHandler( rpc::Message& rpcMessage );
    void invokeHouseModeGetCurrentHandler( rpc::Message& rpcMessage );
    void invokeHouseModeSet( rpc::Message& rpcMessage );
    void invokeHouseModeCancelSwitching( rpc::Message& rpcMessage );
    void invokeHouseModeCancelEntryDelay( rpc::Message& rpcMessage );
    void invokeHouseModeSkipEntryDelay( rpc::Message& rpcMessage );
    void invokeHouseModeSetEntryDelay( rpc::Message& rpcMessage );
    void invokeHouseModeEntryDelayReset( rpc::Message& rpcMessage );
    void invokeHouseModeSetSwitchDelay( rpc::Message& rpcMessage );
    void invokeHouseModeSetAlarmDelay( rpc::Message& rpcMessage );
    void invokeSetDisarmedDefaultForHouseMode( rpc::Message& rpcMessage );
    void invokeHouseModeProtectSet( rpc::Message& rpcMessage );
    void invokeHouseModeProtectButtonsSet( rpc::Message& rpcMessage );
    void invokeHouseModeProtectDevicesAdd( rpc::Message& rpcMessage );
    void invokeHouseModeProtectDevicesRemove( rpc::Message& rpcMessage );
    void invokeAddDisarmedDeviceForHouseMode( rpc::Message& rpcMessage );
    void invokeRemoveDisarmedDeviceForHouseMode( rpc::Message& rpcMessage );
    void invokeHouseModeSetNotifications( rpc::Message& rpcMessage );
    void invokeAddAlarmsOffDeviceForHouseMode( rpc::Message& rpcMessage );
    void invokeRemoveAlarmsOffDeviceForHouseMode( rpc::Message& rpcMessage );
    void invokeAddCamerasOffDeviceForHouseMode( rpc::Message& rpcMessage );
    void invokeRemoveCamerasOffDeviceForHouseMode( rpc::Message& rpcMessage );
    void invokeAddBypassDevicesForHouseMode( rpc::Message& rpcMessage );
    void invokeRemoveBypassDevicesForHouseMode( rpc::Message& rpcMessage );
    void invokeLocalAlarmOff( rpc::Message& rpcMessage );
    void invokeForceDisarm( rpc::Message& rpcMessage );

    void foSetFavoriteImpl( const JsonObject& joParams, JsonObject& joAnswer );
    void foGetFavoriteImpl( const JsonObject& joParams, JsonObject& joAnswer );

    void handleGetScene( rpc::Message& rpcMessage );
    void handleTriggerDevicesList( rpc::Message& rpcMessage );
    void handleGetTimeList( rpc::Message& rpcMessage );
    void handleGetBlocks( rpc::Message& rpcMessage );
    void handleCreateScene( rpc::Message& rpcMessage );
    void handleEditScene( rpc::Message& rpcMessage );
    void handleCloneScene( rpc::Message& rpcMessage );
    void handleDeleteScene( rpc::Message& rpcMessage );
    void handleDeleteAllScenes( rpc::Message& rpcMessage );
    void handleEnableScene( rpc::Message& rpcMessage );
    void handleSetRoom( rpc::Message& rpcMessage );
    void handleSetIcon( rpc::Message& rpcMessage );
    void handleAddNotification( rpc::Message& rpcMessage );
    void handleRemoveNotification( rpc::Message& rpcMessage );
    void handleActionBlockTest( rpc::Message& rpcMessage );
    void handleNameSet( rpc::Message& rpcMessage );
    void handleHouseModeSet( rpc::Message& rpcMessage );
    void handleBlockEnabledSet( rpc::Message& rpcMessage );
    void handleBlockStatusReset( rpc::Message& rpcMessage );
    void handleHubCustomNodesList( rpc::Message& rpcMessage );
    void handleHubScenesBlockDataList( rpc::Message& rpcMessage );
    void handleSceneStatusGet( rpc::Message& rpcMessage );
    void handleSceneMetaSet( rpc::Message& rpcMessage );
    void handleSceneBlockMetaSet( rpc::Message& rpcMessage );

    void handleCreateGlobalScene( rpc::Message& rpcMessage );
    void handleDeleteGlobalScene( rpc::Message& rpcMessage );
    void handleDeleteAllGlobalScenes( rpc::Message& rpcMessage );
    void handleEditGlobalScene( rpc::Message& rpcMessage );
    void handleEnableGlobalScene( rpc::Message& rpcMessage );

    void handleSceneScriptAdd( rpc::Message& rpcMessage );
    void handleSceneScriptSet( rpc::Message& rpcMessage );
    void handleSceneScriptGet( rpc::Message& rpcMessage );
    void handleSceneScriptDelete( rpc::Message& rpcMessage );
    void handleSceneScriptsList( rpc::Message& rpcMessage );
    void handleSceneScriptRun( rpc::Message& rpcMessage );

    void handleCreateScene( rpc::Message& rpcMessage, bool isGlobal );
    void handleDeleteScene( rpc::Message& rpcMessage, bool isGlobal );
    void handleDeleteAllScenes( rpc::Message& rpcMessage, bool isGlobal );
    void handleEditScene( rpc::Message& rpcMessage, bool isGlobal );
    void handleCloneScene( rpc::Message& rpcMessage, bool isGlobal );
    void handleEnableScene( rpc::Message& rpcMessage, bool isGlobal );

    void buildGetGatewaysSettingsResponse(
        rpc::Message& rpcMessage,
        const CoreDRemoteInterface::CoreDRemoteInterface::RawGatewaysSettings& gatewaysSettings );

    void setWrongParamError( rpc::Message& rpcMessage, const std::string& key );
};

}  // namespace Uid
}  // namespace HubPlatform
