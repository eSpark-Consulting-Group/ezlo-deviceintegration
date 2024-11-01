#pragma once

#include "scenes_manager/remote_interface/scenes_base_remote_interface.h"
#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace ScenesManager {
namespace UT {

class FakeScenesManagerInterface final : public BaseRemoteInterface {
public:
    Scene getScene( const SceneId& /*id*/ ) override {
        return Scene();
    }
    Scenes getScenesList( const JsonObject& params ) override {
        return Scenes();
    }
    Scenes getGlobalScenesList( bool ) override {
        return Scenes();
    }
    TriggerDeviceIds getTriggerDevicesList() override {
        return TriggerDeviceIds();
    }
    SceneIds getSceneIds() override {
        return SceneIds();
    }
    JsonArray getTimeList() override {
        return JsonArray();
    }
    JsonArray getBlocksList( const JsonObject& ) override {
        return JsonArray();
    }

    JsonArray getBlocksDataList( const JsonObject& ) override {
        return JsonArray();
    }

    JsonObject createScene( const JsonObject& /*scene*/ ) override {
        return JsonObject();
    }
    JsonObject editScene( const JsonObject& /*message*/ ) override {
        return JsonObject();
    }
    JsonObject cloneScene( const BaseRemoteInterface::SceneId& id, const std::string& name, bool isGlobal ) override {
        return JsonObject();
    }
    JsonObject deleteScene( const SceneId& /*id*/, bool /*isGlobal*/ ) override {
        return JsonObject();
    }
    JsonObject deleteAllScenes( bool /*isGlobal*/ ) override {
        return JsonObject();
    }
    JsonObject enableScene( const SceneId& /*id*/, bool /*enable*/, bool /*isGlobal*/ ) override {
        return JsonObject();
    }
    JsonObject runScene( const SceneId& /*id*/, bool /*isGlobal*/ ) override {
        return JsonObject();
    }
    JsonObject runScene( const SceneId& /*id*/, bool /*isGlobal*/, const std::string& /* block */ ) override {
        return JsonObject();
    }
    JsonObject runScene( const SceneId& /* id */,
                         bool /* global */,
                         const std::string& /* block */,
                         const std::string& /* group */ ) override {
        return JsonObject();
    }
    JsonObject stopScene( const SceneId& id, bool isGlobal, const std::string& block ) {
        return JsonObject();
    }
    JsonObject assignRoomToScene( const SceneId& /*id*/, const RoomId& /*roomId*/ ) override {
        return JsonObject();
    }
    JsonObject assignIconToScene( const SceneId& /*id*/, const Icon& /*icon*/ ) override {
        return JsonObject();
    }
    JsonObject addUserForNotification( const SceneId& /*id*/, const UserId& /*userId*/ ) override {
        return JsonObject();
    }
    JsonObject removeUserFromNotifications( const SceneId& /*id*/, const UserId& /*userId*/ ) override {
        return JsonObject();
    }
    JsonObject setSceneName( const SceneId& /*id*/, const Name& /*name*/ ) override {
        return JsonObject();
    }
    JsonObject setHouseModes( const SceneId& /*id*/, const HouseModes& /*houseModes*/ ) override {
        return JsonObject();
    }
    JsonObject setBlockEnabled( const SceneId& /*id*/, const BlockId& /*blockId*/, bool /*enabled*/ ) override {
        return JsonObject();
    }
    bool setSceneBlocksSource( const std::string& /*sourceId*/ ) override {
        return true;
    }
    bool removeSceneBlocksSource( const std::string& /*sourceId*/ ) override {
        return true;
    }
    JsonObject sceneAllBlocksStatusReset( const SceneId& id ) override {
        return {};
    }
    JsonObject sceneBlockStatusReset( const SceneId& id, const BlockId& blockId ) override {
        return {};
    }
    JsonObject getSceneStatus( const SceneId& id ) override {
        return {};
    }
    JsonObject getSceneState( const SceneId& id ) override {
        return {};
    }
    JsonObject getBlockState( const SceneId& sceneId, const std::string& blockId ) override {
        return {};
    }

    JsonObject setMeta( const BaseRemoteInterface::SceneId& id, const BaseRemoteInterface::Meta& meta ) override {
        return {};
    }

    JsonObject setBlockMeta( const BaseRemoteInterface::SceneId& id,
                             const BaseRemoteInterface::BlockId& blockId,
                             const BaseRemoteInterface::Meta& meta ) override {
        return {};
    }

    bool modifyScenes( const JsonArray& /*commands*/, const JsonArray& /*blockIds*/ ) override {
        return true;
    }

    JsonObject getExpressionValue( const std::string& name, const JsonObject& options ) override {
        return JSON_EMPTY_OBJ;
    }

    JsonObject getScenesInfo( const std::string& id ) override {
        return JSON_EMPTY_OBJ;
    }

    void actionBlockTest( const JsonObject& /*action*/, SendResponseCallback ) override {}
    JsonObject upsertSceneExpression( const std::string& name,
                                      const JsonObject& params,
                                      const std::string& code,
                                      const std::string& valueType,
                                      const Json& metadata,
                                      bool isVariable,
                                      const JsonObject& value ) override {
        return {};
    }
    JsonObject deleteSceneExpression( const std::string& expressionName ) override {
        return {};
    }
    JsonObject listSceneExpressions( bool showCode,
                                     bool strict,
                                     const std::vector< std::string >& filterTypes,
                                     bool hasVariable,
                                     bool variable,
                                     const std::vector< std::string >& names,
                                     bool showValue ) override {
        return {};
    }

    JsonObject updateExpression( const std::string& name,
                                 const JsonObject& params,
                                 const std::string& code,
                                 const std::string& valueType,
                                 const JsonObject& metadata ) override {
        return {};
    }

    JsonObject updateExpressionVariable( const std::string& name,
                                         const JsonObject& value,
                                         const std::string& valueType,
                                         const JsonObject& metadata ) override {
        return {};
    }
};

}  // namespace UT
}  // namespace ScenesManager
}  // namespace HubPlatform
