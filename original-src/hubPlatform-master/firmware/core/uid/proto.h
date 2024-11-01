#pragma once

#include "jsonrpc/message.h"
#include "device_model/h/permission.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "uid/response_builders/filter_data_list.h"

#include "scenes_manager/remote_interface/scenes_remote_interface.h"

#include "device_model/h/permission.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Uid {

class Service;

class Proto {
public:
    Proto( Service& uiService, HubPlatform::ScenesManager::RemoteInterface& ri );

    bool processCmd( rpc::Message& rpcMessage );
    virtual void processBroadcast( const Services::Message& message );

    std::string getRegisterMethod();
    size_t getNmaPingInterval();

protected:
    Service& getService();

private:
    void unknown( rpc::Message& rpcMessage );
    void ping( rpc::Message& rpcMessage );
    void setDescription( rpc::Message& rpcMessage );
    void getWifiList( rpc::Message& rpcMessage );
    void connectWifi( rpc::Message& rpcMessage );
    void deleteWifi( rpc::Message& rpcMessage );
    void getStatusWifi( rpc::Message& rpcMessage );
    void resetWifi( rpc::Message& rpcMessage );
    void getEzloIp( rpc::Message& rpcMessage );
    void getEzloMac( rpc::Message& rpcMessage );
    void setEzloLogLevel( rpc::Message& rpcMessage );
    void setEzloLogSettings( rpc::Message& rpcMessage );
    void hubLogSend( rpc::Message& rpcMessage );
    void getEzloVersion( rpc::Message& rpcMessage );
    void requestNewNMAServer( rpc::Message& rpcMessage );
    void getDevices( rpc::Message& rpcMessage );
    void getItems( rpc::Message& rpcMessage );
    void getDeviceSetting( rpc::Message& rpcMessage );
    void setDeviceSettingValue( rpc::Message& rpcMessage );
    void setDeviceSettingDictionaryValue( rpc::Message& rpcMessage );
    void resetDeviceSetting( rpc::Message& rpcMessage );
    void checkDevice( rpc::Message& rpcMessage );
    void getHubInfo( rpc::Message& rpcMessage );
    void setServiceNotifications( rpc::Message& rpcMessage );
    void handleSceneList( rpc::Message& rpcMessage );
    void handleGlobalSceneList( rpc::Message& rpcMessage );
    void handleRunScene( rpc::Message& rpcMessage );
    void handleStopScene( rpc::Message& rpcMessage );
    void handleSceneExpressionsSet( rpc::Message& rpcMessage );
    void handleSceneExpressionsDelete( rpc::Message& rpcMessage );
    void handleSceneExpressionsList( rpc::Message& rpcMessage );
    void handleCloudSubscriptionCreate( rpc::Message& rpcMessage );
    void handleCloudSubscriptionUpdate( rpc::Message& rpcMessage );
    void handleCloudSubscriptionDelete( rpc::Message& rpcMessage );
    void handleCloudSubscriptionsList( rpc::Message& rpcMessage );
    void handleCloudSubscriptionDispatch( rpc::Message& rpcMessage );
    void handleHubVideoForward( rpc::Message& rpcMessage );
    void handleHubVidooElementsEvent( rpc::Message& rpcMessage );
    void handleDeviceGroupCreate( rpc::Message& rpcMessage );
    void handleDeviceGroupDelete( rpc::Message& rpcMessage );
    void handleDeviceGroupUpdate( rpc::Message& rpcMessage );
    void handleDeviceGroupsList( rpc::Message& rpcMessage );
    void handleDeviceGroupGet( rpc::Message& rpcMessage );
    void handleDeviceGroupDeviceAdd( rpc::Message& rpcMessage );
    void handleDeviceGroupDeviceRemove( rpc::Message& rpcMessage );
    void handleItemGroupCreate( rpc::Message& rpcMessage );
    void handleItemGroupDelete( rpc::Message& rpcMessage );
    void handleItemGroupUpdate( rpc::Message& rpcMessage );
    void handleItemGroupsList( rpc::Message& rpcMessage );
    void handleItemGroupsGet( rpc::Message& rpcMessage );
    void handleDeviceItemGroupExpand( rpc::Message& rpcMessage );
    void handleDeviceGroupsFind( rpc::Message& rpcMessage );
    void handlePolicyCreate( rpc::Message& rpcMessage );
    void handlePolicyDelete( rpc::Message& rpcMessage );
    void handlePolicyUpdate( rpc::Message& rpcMessage );
    void handlePolicyGet( rpc::Message& rpcMessage );
    void handlePolicyList( rpc::Message& rpcMessage );

    void handleHubReboot( rpc::Message& rpcMessage );
    void handleHubPoweroff( rpc::Message& rpcMessage );
    void handleDataList( rpc::Message& rpcMessage );
    void handleHubRoomList( rpc::Message& rpcMessage );

    void handleRunGlobalScene( rpc::Message& rpcMessage );

    void handleRunScene( rpc::Message& rpcMessage, bool isGlobal );
    void handleStopScene( rpc::Message& rpcMessage, bool isGlobal );

    void handleCloudControllerResponse( rpc::Message& rpcMessage );
    void handleBridgeControllerConnectionSet( rpc::Message& rpcMessage );
    void handleBridgeConnectionsList( rpc::Message& rpcMessage );
    void handleBridgeControllersList( rpc::Message& rpcMessage );

    void handleHubOfflineAnonymousAccessEnabledSet( rpc::Message& rpcMessage );
    void handleHubOfflineInsecureAccessEnabledSet( rpc::Message& rpcMessage );
    void handleHubOfflineCredentialsUpdated( rpc::Message& rpcMessage );

    void handleHubCloudRegisterRepeat( rpc::Message& rpcMessage );

    void handleHubFeaturesList( rpc::Message& rpcMessage );
    void handleHubFeatureStatusSet( rpc::Message& rpcMessage );
    void handleHubSettingsList( rpc::Message& rpcMessage );
    void handleSettingValueSet( rpc::Message& rpcMessage );

    void handleSshTunnelSupportEnabledSet( rpc::Message& rpcMessage );

    void invokeHubSoftwareHandler( const std::string& method, rpc::Message& rpcMessage );
    void invokeNetworkHandler( const std::string& method, rpc::Message& rpcMessage );
    void invokeProvisionInfoHandler( const std::string& method, rpc::Message& rpcMessage );
    void invokeHubAddonsHandler( const std::string& method, rpc::Message& rpcMessage );
    void invokeHubBackupHandler( const std::string& method, rpc::Message& rpcMessage );
    bool invokeHubHandler( const std::string& method, rpc::Message& rpcMessage );

    void buildGetDevicesResponse( rpc::Message& rpcMessage, const std::vector< JsonObject >& devices );
    static void buildDataListDevicesResponse( rpc::Message& rpcMessage,
                                              const std::vector< JsonObject >& devices,
                                              FilterDataListI* filter );

    bool redirectCustomServiceMessage( rpc::Message& rpcMessage );
    bool hubLogCommand( const std::string& id, const std::string& method, const JsonObject& answer );
    bool cloudLogCommand( std::string& id, const std::string& method, const JsonObject& message, JsonObject& answer );

    void handleContactIdsList( rpc::Message& rpcMessage );
    void handleSecurityConfigList( rpc::Message& rpcMessage );

    void filterBypassDevices( std::vector< JsonObject >& devices );

    void handleCloudLocalDataUpdate( rpc::Message& rpcMessage );

    void handleNotificationsSmtpSettingsSet( rpc::Message& rpcMessage );
    void handleNotificationsSmtpSettingsGet( rpc::Message& rpcMessage );
    void handleNotificationsSmtpSend( rpc::Message& rpcMessage );

    void handleHubOfflineCloudDataInfoGet( rpc::Message& rpcMessage );

protected:
    void setItemValue( rpc::Message& rpcMessage );
    void changeItemValueLevelByPercent( rpc::Message& rpcMessage );
    void updateDeviceFirmware( rpc::Message& rpcMessage );
    void getServiceInfo( rpc::Message& rpcMessage );

protected:
    /* @brief find and execute request handler. Does not error message in case method is not found.
     * @return is handler found and executed
     * */
    virtual bool invokeHandler( const std::string& method, rpc::Message& message );

    /* @brief find and execute request handler. Uses method name is separated by point (example: method.submethod).
     * Does not error message in case method is not found.
     * @return is handler found and executed
     * */
    virtual bool invokeExtendedHandler( const std::string& method,
                                        const std::string& submethod,
                                        rpc::Message& message );

    static JsonObject prepareMessage( JsonObject params, const std::string& operationId );
    static void buildGetItemsResponse( rpc::Message& rpcMessage, const std::vector< JsonObject >& items );
    static void buildDataListItemsResponse( rpc::Message& rpcMessage,
                                            const std::vector< JsonObject >& devices,
                                            FilterDataListI* filter );

    static void buildGetDeviceSettingsResponse(
        rpc::Message& rpcMessage,
        const CoreDRemoteInterface::CoreDRemoteInterface::DeviceSettings& deviceSettings );

    static void buildDataListCollectionResponse( rpc::Message& rpcMessage,
                                                 const std::vector< JsonObject >& records,
                                                 const std::string& collectionName,
                                                 const std::string& idFieldName,
                                                 FilterDataListI* filter );

    static void buildDataListCollectionResponse( rpc::Message& rpcMessage,
                                                 const JsonObject& records,
                                                 const std::string& collectionName,
                                                 FilterDataListI* filter );

    HubPlatform::ScenesManager::RemoteInterface& getScenesManager();

private:
    Service& m_uiService;
    HubPlatform::ScenesManager::RemoteInterface& m_scenesRemoteInterface;
};

}  // namespace Uid
}  // namespace HubPlatform
