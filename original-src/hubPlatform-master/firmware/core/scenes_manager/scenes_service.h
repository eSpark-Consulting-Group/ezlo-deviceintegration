#pragma once

#include "service_interfaces/service.h"
#include "scenes_manager/scenes_engine/scene_manager.h"
#include "scenes_manager/scenes_processor/scene_request_handler.h"
#include "scenes_manager/scenes_processor/controller_interface/controller_factory.h"
#include "cored/remote_interface/cored_interface.h"
#include "extensions_manager/remote_interface/em_remote_interface.h"

namespace SceneEngine {
class IClock;
class Environment;
class StatesRegistry;
class RegistryFlushScheduler;
}  // namespace SceneEngine

namespace HubPlatform {

namespace UT {
class ScenesAPIFixture;
}  // namespace UT

namespace ScenesManager {

class Service : public Services::Service {
public:
    using ControllerId = std::string;

    Service();
    ~Service() override;

    int restoreBackup( const std::string& tempBackupFolder ) override;

protected:
    void setUp() override;
    void tearDown() override;

    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    bool handleAsyncRequestFromBus( const JsonObject& request,
                                    MessageSubClasses requestType,
                                    const std::string& uuid,
                                    const std::string& sender ) override;

protected:
    Service( const std::string& name,
             std::unique_ptr< Services::BusWrapper > impl,
             std::unique_ptr< IDbClient > dbClient );

private:
    using DictionaryElementKey = std::string;
    using DictionaryElementValue = Json;

    struct DictionaryElement {
        DictionaryElementKey key;
        DictionaryElementValue value;
    };

    using DetectedPerson = std::string;
    using DetectedObject = std::string;
    using DetectedQRCode = std::string;
    using DetectedPersons = std::set< DetectedPerson >;
    using DetectedObjects = std::set< DetectedObject >;
    using DetectedQRCodes = std::set< DetectedQRCode >;

    struct DetectedItems {
        DetectedPersons persons;
        DetectedObjects objects;
        DetectedQRCodes qrCodes;
    };

    JsonObject handleCreateScene( const JsonObject& params );
    JsonObject handleEditScene( const JsonObject& params );
    JsonObject handleDeleteScene( const JsonObject& params );
    JsonObject handleDeleteAllScenes( const JsonObject& params );
    JsonObject handleSwitchScene( const JsonObject& params );
    JsonObject handleCloneScene( const JsonObject& params );
    JsonObject handleRunScene( const JsonObject& params );
    JsonObject handleStopScene( const JsonObject& params );
    JsonObject handleSetTestClock( const JsonObject& params );
    JsonObject handleRoomSet( const JsonObject& params );
    JsonObject handleRoomRemoved( const JsonObject& params );
    JsonObject handleNotificationAdd( const JsonObject& params );
    JsonObject handleNotificationRemove( const JsonObject& params );
    JsonObject handleTimeList( const JsonObject& params );
    JsonObject handleLockRequest( const JsonObject& request );
    JsonObject handleUnlockRequest( const JsonObject& request );
    JsonObject handleBackupCreateRequest( const JsonObject& request );

    void handleEventMessage( const int msgClass, const int subClass, const JsonObject& obj, const std::string& sender );
    void handleItemUpdate( const JsonObject& message );
    void handleExtendedItemUpdate( const JsonObject& message );
    void handleItemDictionaryUpdate( const JsonObject& message );
    void handleHouseModeUpdate( const JsonObject& item );
    void handleHouseModePhaseUpdate( const JsonObject& message );
    void handleHouseModeSwitchToUpdate( const JsonObject& message );
    void handleBatteryInfoUpdate( const JsonObject& message );
    void handleSystemTimeUpdate( const JsonObject& );
    void handleDeviceUpdated( const JsonObject& );
    void handleMainNetworkInterface( const JsonObject& );
    void handleNetworkChanged( const JsonObject& );
    void handleCloudConnectionState( bool isConnected, const JsonObject& message );
    void handleFirmwareUpdate( const JsonObject& message );
    void handleSystemDateTimeChanged( const JsonObject& message );
    void handleDeviceRemoved( const JsonObject& message );
    void handleHubFeatureUpdated( const JsonObject& message );

    void processHotzoneUpdate( const JsonObject& message );
    void processGenericExtendedItemUpdate( const JsonObject& message );

    void processActionRunRequest( const std::string& name,
                                  const SceneEngine::Action& action,
                                  SceneEngine::ActionExecutorPtr actionExecutorPtr );

    DictionaryElement getDictionaryNewElement( const Json& message ) const;
    DictionaryElement getDictionaryOldElement( const Json& message ) const;
    DictionaryElement getDictionaryElement( const std::string& field, const Json& message ) const;

    ControllerId getControllerId( const Json& obj );

    SceneEngine::SceneManager& getSceneManager();

    void handleDeviceGroupChanged( const JsonObject& message );
    void handleItemGroupChanged( const JsonObject& message );
    JsonObject createBroadcastMessage( const int msgClass, const int subClass, const JsonObject& obj );

private:
    std::unique_ptr< CoreDRemoteInterface::CoreDInterface > m_core;
    std::unique_ptr< ExtensionsManager::RemoteInterface > m_extensionsManager;
    std::unique_ptr< ExternalControllers::RemoteInterface > m_extControllerInterface;
    std::unique_ptr< SceneEngine::IClock > m_clock;
    std::unique_ptr< SceneEngine::ItemCacheBase > m_itemCache;
    std::unique_ptr< SceneEngine::Expressions::ExpressionManager > m_expressions;
    std::unique_ptr< SceneEngine::StatesRegistry > m_statesRegistry;
    std::unique_ptr< SceneEngine::RegistryFlushScheduler > m_registryFlushScheduler;
    std::unique_ptr< CoreD::DeviceItemGroupManager > m_deviceItemGroupManager;
    std::unique_ptr< SceneEngine::SceneManager > m_scenesManager;
    std::unique_ptr< SceneEngine::Environment > m_env;
    std::unique_ptr< ScenesRequestHandlerInterface > m_scenesHandler;
    std::unique_ptr< ControllerFactory > m_controllerFactory;
    std::atomic_bool m_subscribedOnCloudAnswers{ false };
    ControllerId m_ownControllerId;

    struct AsyncRequestInfo {
        AsyncRequestInfo( JsonObject request, std::string uuid, std::string sender );

        JsonObject m_request;
        std::string m_uuid;
        std::string m_sender;
    };

    std::unique_ptr< AsyncRequestInfo > m_veraBackupRequestInfo;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
