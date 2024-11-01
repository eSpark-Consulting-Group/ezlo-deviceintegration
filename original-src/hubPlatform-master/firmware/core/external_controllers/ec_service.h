#pragma once

#include "external_controllers/ec_messenger.h"
#include "external_controllers/ec_core.h"
#include "uid/icc_interface/icc_remote_interface.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace ExternalControllers {

class Service final : public Services::Service, public Messenger {
public:
    Service();

    int restoreBackup( const std::string& tempBackupFolder ) override;

protected:
    void setUp() override;

    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;

    void sendInternalBroadcast( MessageSubClasses eventType, const JsonObject& broadcast ) override;

    void registerControllerConnection( const std::string& controllerId ) override;
    void unregisterControllerConnection( const std::string& controllerId ) override;

private:
    void handleEventFromBus( const Services::Message& message );
    void handleBroadcast( const JsonObject& data );

    JsonObject handleLockRequest( const JsonObject& json );
    JsonObject handleUnlockRequest( const JsonObject& json );
    JsonObject handleBackupCreateRequest( const JsonObject& json );

    void handleItemUpdatedBroadcast( const std::string& controllerId, const JsonObject& result );
    void handleItemDictionaryUpdatedBroadcast( const std::string& controllerId, const JsonObject& result );
    void handleItemRemovedBroadcast( const std::string& controllerId, const JsonObject& result );
    void handleDeviceUpdatedBroadcast( const std::string& controllerId, const JsonObject& result );
    void handleDeviceRemovedBroadcast( const std::string& controllerId, const JsonObject& result );

    void handleControllerConnected( const JsonObject& data );
    void handleSubscribe( const JsonObject& json );
    void handleUnsubscribe( const JsonObject& json );

    void requestItemsList( const std::string& controllerId ) override;
    void handleItemsListResponse( const std::string& controllerId, const JsonObject& response );

    void requestDevicesList( const std::string& controllerId ) override;
    void handleDevicesListResponse( const std::string& controllerId, const JsonObject& response );

private:
    Service( const std::string& name,
             std::unique_ptr< Services::BusWrapper > impl,
             std::unique_ptr< IDbClient > dbClient );

private:
    std::unique_ptr< Core > m_core;
    InterControllerCommunication::RemoteInterface m_icc;
    std::chrono::seconds m_reaskOnErrorDelay;

    struct ControllerRequestsInfo {
        ControllerRequestsInfo()
        : pendingItmesList( false )
        , updateItemsList( false )
        , pendingDevicesList( false )
        , updateDevicesList( false ) {}

        Services::PostponedTaskPtr reaskItemsTask;
        Services::PostponedTaskPtr reaskDevicesTask;

        bool pendingItmesList;
        bool updateItemsList;
        bool pendingDevicesList;
        bool updateDevicesList;
    };
    std::unordered_map< std::string, ControllerRequestsInfo > m_controllersRequests;
};

}  // namespace ExternalControllers
}  // namespace HubPlatform
