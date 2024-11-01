#pragma once

#include "cloud_devices/subscription_manager/subscription_helper.h"
#include "cloud_devices/subscription_manager/subscription_manager.h"
#include "cored/remote_interface/cored_interface.h"
#include "service_interfaces/service.h"
#include "cloud_devices/local_notifications/local_notifications.h"

#if defined( __WITH_CLOUD_DEVICES_LEGACY_CODE__ )
#include "addon_interfaces/addon.h"
#include "config_reader/config_reader.h"
#endif  // __WITH_CLOUD_DEVICES_LEGACY_CODE__

#include <vector>

namespace HubPlatform {
namespace CloudDevices {

class CloudDevicesService final : public Services::Service {
public:
    static const char* const Name;

public:
    CloudDevicesService();

    int restoreBackup( const std::string& tempBackupFolder ) override;

private:
    CloudDevicesService( const std::string& name,
                         std::unique_ptr< Services::BusWrapper > impl,
                         std::unique_ptr< IDbClient > dbClient );

    void setUp() override;
    void tearDown() override;

    void handleMessageFromBus( const Services::Message& message, const std::string& /*sender*/ ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleLegacyMethodCall( const JsonObject& json, const std::string& /*sender*/ ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;
    bool handleEventFromBus( const Services::Message& message );
    void handleCloudEventFromBus( const Services::Message& message );

    JsonObject doHandleRequestFromBus( const JsonObject& request, MessageSubClasses requestType );

private:
    JsonObject handleCloudSubscriptionCreate( const JsonObject& request );
    JsonObject handleCloudSubscriptionDelete( const JsonObject& request );
    JsonObject handleCloudSubscriptionUpdate( const JsonObject& request );
    JsonObject handleCloudSubscriptionDispatch( const JsonObject& request );

    void handleCloudRegistered();
    void handleCloudUnregistered();

    void forgetExistingAsyncRequests();

    void startDelayedSubscriptionRequest();
    void cancelDelayedSubscriptionRequest();

    void checkDeviceCategory( const Services::Message& message );
    void checkItemByDeviceCategory( const Services::Message& message );

    void handleAbstractDeviceAdded( const Services::Message& message );
    void handleAbstractItemUpdated( const Services::Message& message );

    void handleGatewayCommandRequest( const JsonObject& params );

#if defined( __WITH_CLOUD_DEVICES_LEGACY_CODE__ )
    void handleSendCommandRequest( const JsonObject& params );
    int getExistingDevicesDataVersion() const;
    bool setCurrentDevicesDataVersion( int version );

    void requestControllerAbstractsList();
    void handleControllerAbstractsListTimeout();
    void handleControllerAbstractsListResponse( const JsonObject& response );

    bool checkNewVersion( const JsonObject& result );
    void handleCloudAbstractsUpdated( const JsonObject& result );
    void handleCloudAbstractsAdded( const JsonObject& result );
    void handleCloudAbstractsDeleted( const JsonObject& result );

    void handleSendCommandTimeout( const std::string& commandId );
    void handleSendCommandResponse( const std::string& commandId, const JsonObject& response );

private:
    Json getDeviceDiff( const std::string& deviceId, const Json& newDeviceJson ) const;
    void updateDevicesList( int newVersion, const Json& newDevices );

private:
    bool deviceExists( const std::string& deviceId ) const;
    Json getDevice( const std::string& deviceId ) const;

    void emitAllDevicesReachableStateUpdated( int newState );

    void addDevice( const JsonObject& deviceJson );
    void updateDevice( const std::string& deviceId, const JsonObject& deviceJson );
    void deleteDevice( const std::string& deviceId );

private:
    ConfigReader m_configReader;
#endif  // __WITH_CLOUD_DEVICES_LEGACY_CODE__

private:
    std::unique_ptr< CoreDRemoteInterface::CoreDInterface > m_core;
    std::unique_ptr< SubscriptionHelper > m_subscriptionHelper;
    std::unique_ptr< SubscriptionManager > m_subscriptionManager;
    std::chrono::seconds m_cloudRequestTimeout;
    std::chrono::seconds m_reaskOnErrorDelay;
    bool m_pendingControllerAbstractsList;
    bool m_updateControllerAbstractsList;
    HubPlatform::Services::PostponedTaskPtr m_subscriptionDelayingTask;
    std::unique_ptr< LocalNotifications > m_localNotifications;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
