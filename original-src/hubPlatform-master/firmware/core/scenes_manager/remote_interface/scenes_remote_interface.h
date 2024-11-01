#pragma once

#include "scenes_manager/remote_interface/scenes_base_remote_interface.h"
#include "service_interfaces/service.h"

#include <memory>

namespace HubPlatform {
namespace Services {
class Service;
}
namespace ScenesManager {

class RemoteInterface final : public BaseRemoteInterface {
public:
    RemoteInterface( Services::Service& hostService );

    Scene getScene( const SceneId& id ) override;
    Scenes getScenesList( const JsonObject& params ) override;
    Scenes getGlobalScenesList( bool onlyId ) override;
    TriggerDeviceIds getTriggerDevicesList() override;
    SceneIds getSceneIds() override;
    JsonArray getTimeList() override;
    /// @param deviceIds
    /// @param blockType possible values are `when` and `then`
    JsonArray getBlocksList( const JsonObject& message ) override;
    JsonObject getBlocksDataList( const JsonObject& message ) override;

    JsonObject createScene( const JsonObject& scene ) override;
    /// @param sceneId
    /// @param eo new scene
    JsonObject editScene( const JsonObject& message ) override;
    JsonObject deleteScene( const SceneId& id, bool isGlobal ) override;
    JsonObject deleteAllScenes( bool isGlobal ) override;
    JsonObject enableScene( const SceneId& id, bool enable, bool isGlobal ) override;
    JsonObject cloneScene( const BaseRemoteInterface::SceneId& id, const std::string& name, bool isGlobal ) override;
    JsonObject runScene( const SceneId& id, bool isGlobal ) override;
    JsonObject runScene( const SceneId& id, bool isGlobal, const std::string& block ) override;
    JsonObject runScene( const SceneId& id,
                         bool isGlobal,
                         const std::string& block,
                         const std::string& group ) override;
    JsonObject stopScene( const SceneId& id, bool isGlobal, const std::string& block ) override;
    JsonObject stopScene( const SceneId& id, bool isGlobal, const std::string& block, const std::string& blockId );
    JsonObject assignRoomToScene( const SceneId& id, const RoomId& roomId ) override;
    JsonObject assignIconToScene( const SceneId& id, const Icon& icon ) override;
    JsonObject addUserForNotification( const SceneId& id, const UserId& userId ) override;
    JsonObject removeUserFromNotifications( const SceneId& id, const UserId& userId ) override;
    JsonObject setSceneName( const SceneId& id, const Name& name ) override;
    JsonObject setHouseModes( const SceneId& id, const HouseModes& houseModes ) override;
    JsonObject setBlockEnabled( const SceneId& id, const BlockId& blockId, bool enabled ) override;

    bool setSceneBlocksSource( const std::string& sourceId ) override;
    bool removeSceneBlocksSource( const std::string& sourceId ) override;
    JsonObject sceneAllBlocksStatusReset( const SceneId& id ) override;
    JsonObject sceneBlockStatusReset( const SceneId& id, const BlockId& blockId ) override;
    JsonObject getSceneStatus( const SceneId& id ) override;

    JsonObject getSceneState( const SceneId& id ) override;
    JsonObject getBlockState( const SceneId& sceneId, const std::string& blockId ) override;
    JsonObject setMeta( const BaseRemoteInterface::SceneId& id, const BaseRemoteInterface::Meta& meta ) override;
    JsonObject setBlockMeta( const BaseRemoteInterface::SceneId& id,
                             const BaseRemoteInterface::BlockId& blockId,
                             const BaseRemoteInterface::Meta& meta );
    /* @param:  commands ( array of jsons )
     *              possible commands:
     *              [
     *                  {
     *                       "command": "replace_token",
     *                       "to_replace": "string",
     *                       "replace_by": "string",
     *                       "block_ids": [ "array of strings" ]  // optional
     *                  },
     *                  {
     *                       "command": "replace_language_text",
     *                       "language_tag": "string",
     *                       "replace_by_text": "string",
     *                       "block_ids": [ "array of strings" ] // optional
     *                  }
     *              ]
     * @param blockIds array of block ids. If this argument is empty then commands are affected to all blocks
     */
    bool modifyScenes( const JsonArray& commands, const JsonArray& blockIds ) override;
    void actionBlockTest( const JsonObject& action, SendResponseCallback sendResponseCallback ) override;
    JsonObject getExpressionValue( const std::string& name, const JsonObject& options ) override;
    JsonObject getScenesInfo( const std::string& id ) override;

    JsonObject upsertSceneExpression( const std::string& name,
                                      const JsonObject& params,
                                      const std::string& code,
                                      const std::string& valueType,
                                      const Json& metadata,
                                      bool variable,
                                      const JsonObject& value ) override;
    JsonObject deleteSceneExpression( const std::string& name ) override;
    JsonObject listSceneExpressions( bool showCode,
                                     bool strict,
                                     const std::vector< std::string >& filterTypes,
                                     bool hasVariable,
                                     bool variable,
                                     const std::vector< std::string >& names,
                                     bool showValue ) override;

    JsonObject updateExpression( const std::string& name,
                                 const JsonObject& params,
                                 const std::string& code,
                                 const std::string& valueType,
                                 const JsonObject& metadata ) override;

    JsonObject updateExpressionVariable( const std::string& name,
                                         const JsonObject& value,
                                         const std::string& valueType,
                                         const JsonObject& metadata ) override;
    JsonObject checkScenesFilterParams( const JsonObject& params );

private:
    std::string temperatureScale() const;
    std::pair< bool, Json > modifySceneScales( const Json& scene, std::string const& defaultScale );
    void copyFieldIfPresent( const Json& source, const std::string& key, Json& destination );

private:
    Services::Service& m_hostService;
    IDbClient& m_dbClient;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
