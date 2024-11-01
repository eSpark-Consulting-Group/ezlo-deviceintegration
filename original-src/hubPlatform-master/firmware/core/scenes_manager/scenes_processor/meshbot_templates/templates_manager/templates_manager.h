#pragma once

#include "scenes_manager/scenes_processor/meshbot_templates/types/meshbot_template.h"
#include "scenes_manager/scenes_processor/meshbot_templates/types/meshbot_trigger.h"
#include "scenes_manager/scenes_processor/meshbot_templates/types/template_binding.h"
#include "json_utils/json_fwd.h"

#include <string>
#include <vector>
#include <memory>
#include <tuple>

class IDbClient;

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {

class MeshbotBuilder;

class TemplatesManager {
public:
    class HandlerInterface {
    public:
        virtual std::vector< Types::MeshbotTemplate > getMeshbotTemplates() const = 0;
        virtual std::vector< Types::MeshbotTemplate > getMeshbotTemplates( int meshbotsVersion,
                                                                           int fileVersion ) const = 0;
        virtual std::vector< Json > getDevices() const = 0;
        virtual std::vector< Json > getItems() const = 0;
        virtual std::string getControllerSerial() const = 0;
        virtual int getTcdsGeneralDataVersion() const = 0;
        virtual int getTcdsMeshbotsDataVersion() const = 0;
        virtual void switchGeneralDataVersion( int generalDataVersion, int fileVersion ) = 0;
        virtual void switchMeshbotsVersion( int meshbotsVersion, int fileVersion ) = 0;

        virtual void createScene( Json scene ) = 0;
        virtual void deleteScene( const std::string& sceneId ) = 0;
        virtual bool isSceneModified( const std::string& sceneId ) const = 0;
    };

    TemplatesManager( IDbClient& dbClient, std::unique_ptr< HandlerInterface > handler );
    ~TemplatesManager();

    void handleSyncMeshbotTemplates();
    void handleDeviceUpdated( const std::string& deviceId );
    void handleItemUpdated( const std::string& itemId );

    using Error = std::string;
    Error handleMigrateMeshbotTemplates( const int generalDataVersion,
                                         const int generalDataFileVersion,
                                         const int meshbotsDataVersion,
                                         const int meshbotsFileVersion );

private:
    using MeshbotUUID = std::string;
    using DeviceId = std::string;
    using ItemId = std::string;
    using BindingKey = std::tuple< MeshbotUUID, DeviceId, ItemId >;

private:
    void syncMeshbotTemplates();
    bool createScene( const MeshbotBuilder& meshbotBuilder,
                      const std::string& sceneId,
                      const Types::MeshbotTemplate& meshbotTemplate,
                      const Types::MeshbotTrigger& trigger );
    void removeBinding( const Types::TemplateBinding& binding );
    void updateBindings( const std::vector< Types::TemplateBinding >& bindings );

private:
    IDbClient& m_dbClient;
    std::unique_ptr< HandlerInterface > m_handler;
};

}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
