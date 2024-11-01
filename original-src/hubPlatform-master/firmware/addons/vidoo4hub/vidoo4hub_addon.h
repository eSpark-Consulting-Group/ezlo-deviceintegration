#pragma once
#include "addon_interfaces/addon.h"

#include "vidoo4hub/core_manager/core_manager.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "vidoo4hub/stream_manager/stream_manager.h"
#include "vidoo4hub/camera_manager/camera_manager.h"
#include "vidoo4hub/vidoo_manager/vidoo_manager.h"
#include "vidoo4hub/vidoo_mediator/vidoo_mediator.h"
#include "vidoo4hub/vidoo_manager/vidoo_manager_resources.h"
#include "vidoo4hub/vidoo_device/vidoo_device.h"
#include "vidoo4hub/dictionary_manager/dictionary_manager.h"

#include "vidoo4hub/vidoo_client/vidoo_client_interface.h"

namespace HubPlatform {

class Vidoo4HubAddon final : public Addons::Addon {
public:
    Vidoo4HubAddon();
    ~Vidoo4HubAddon();

    CommonUtils::AddonVersion getVersion() const override;
    int restoreBackup( const std::string& tempBackupFolder ) override;

private:
    void handleMethodCall( const JsonObject& request,
                           MessageSubClasses requestType,
                           const std::string& sender ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleLegacyMethodCall( const JsonObject& json, const std::string& /*sender*/ ) override;

private:
    void handleEventMessage( const int subClass, const JsonObject& obj );
    void handleVidooMessage( const JsonObject& msg );

    std::string ensureGateway();

private:
    void setUp() override;
    void tearDown() override;

private:
    std::unique_ptr< CoreDRemoteInterface::CoreDInterface > m_core;
    std::unique_ptr< Vidoo4Hub::CoreManager > m_coreManager;
    std::unique_ptr< Vidoo4Hub::VidooDevice > m_vidooDevice;
    std::unique_ptr< Vidoo4Hub::StreamManager > m_streamManager;
    std::unique_ptr< Vidoo4Hub::CameraManager > m_cameraManager;
    std::unique_ptr< Vidoo4Hub::DictionaryManager > m_dictionaryManager;
    std::unique_ptr< Vidoo4Hub::VidooManagerResources > m_vidooMangerResources;
    std::unique_ptr< Vidoo4Hub::VidooManager > m_vidooManager;
    std::unique_ptr< Vidoo4Hub::IVidooClient > m_vidooClient;
    std::unique_ptr< Vidoo4Hub::VidooMediator > m_vidooMediator;
};

}  // namespace HubPlatform
