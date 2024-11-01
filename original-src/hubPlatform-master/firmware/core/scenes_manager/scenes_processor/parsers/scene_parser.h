#pragma once

#include "scenes_manager/scenes_engine/scene.h"
#include "cored/remote_interface/cored_interface.h"
#include "json_utils/json_utils.h"
#include <memory>

namespace HubPlatform {
namespace ScenesManager {

class SceneParserInterface {
public:
    using ScenePtr = std::shared_ptr< SceneEngine::Scene >;

    virtual ~SceneParserInterface() = default;
    virtual ScenePtr create( const Json& sceneJsonObject ) = 0;
    virtual ScenePtr parse( const Json& sceneJsonObject, bool isEditing = false ) = 0;

    static const std::size_t SceneIdMaxSize = 64;
    static const std::size_t IconMaxSize = 128;
    static const std::size_t NameMaxSize = 128;
};

class SceneParser : public SceneParserInterface {
public:
    explicit SceneParser( CoreDRemoteInterface::CoreDInterface& core );

    ScenePtr create( const Json& sceneJsonObject ) override;
    ScenePtr parse( const Json& sceneJsonObject, bool isEditing = false ) override;

private:
    bool setUserNotifications( const JsonObject& sceneJsonObject, SceneEngine::Scene& scene );
    bool setHouseModes( const JsonObject& sceneJsonObject, SceneEngine::Scene& scene );
    void setRoomName( SceneEngine::Scene& scene );
    void processGeneratorField( const JsonObject& sceneJsonObject, JsonObject& generator, bool isEditing );

private:
    CoreDRemoteInterface::CoreDInterface& m_core;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
