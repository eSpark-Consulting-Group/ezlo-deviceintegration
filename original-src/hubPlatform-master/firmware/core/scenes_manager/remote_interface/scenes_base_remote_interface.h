#pragma once

#include "json_utils/json_fwd.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace ScenesManager {

class BaseRemoteInterface {
public:
    using Scene = JsonObject;
    using Scenes = std::vector< Scene >;
    using DeviceId = std::string;
    using TriggerDeviceIds = std::set< DeviceId >;
    using DeviceIds = JsonArray;
    using SceneId = std::string;
    using SceneIds = std::vector< SceneId >;
    using RoomId = std::string;
    using UserId = std::string;
    using BlockId = std::string;
    using BlockType = std::string;
    using Icon = std::string;
    using Name = std::string;
    using HouseModes = JsonArray;
    using Meta = JsonObject;
    using SendResponseCallback = std::function< void( const JsonObject& response ) >;

    virtual ~BaseRemoteInterface() = default;

    virtual Scene getScene( const SceneId& id ) = 0;
    virtual Scenes getScenesList( const JsonObject& params ) = 0;
    virtual Scenes getGlobalScenesList( bool onlyId ) = 0;
    virtual TriggerDeviceIds getTriggerDevicesList() = 0;
    virtual SceneIds getSceneIds() = 0;
    virtual JsonArray getTimeList() = 0;
    virtual JsonArray getBlocksList( const JsonObject& message ) = 0;

    virtual JsonObject getBlocksDataList( const JsonObject& message ) = 0;
    virtual JsonObject createScene( const JsonObject& scene ) = 0;
    virtual JsonObject editScene( const JsonObject& message ) = 0;
    virtual JsonObject deleteScene( const SceneId& id, bool isGlobal ) = 0;
    virtual JsonObject deleteAllScenes( bool isGlobal ) = 0;
    virtual JsonObject enableScene( const SceneId& id, bool enable, bool isGlobal ) = 0;
    virtual JsonObject cloneScene( const BaseRemoteInterface::SceneId& id, const std::string& name, bool isGlobal ) = 0;
    virtual JsonObject runScene( const SceneId& id, bool isGlobal ) = 0;
    virtual JsonObject runScene( const SceneId& id, bool isGlobal, const std::string& block ) = 0;
    virtual JsonObject runScene( const SceneId& id,
                                 bool isGlobal,
                                 const std::string& block,
                                 const std::string& group ) = 0;
    virtual JsonObject stopScene( const SceneId& id, bool isGlobal, const std::string& block ) = 0;
    virtual JsonObject assignRoomToScene( const SceneId& id, const RoomId& roomId ) = 0;
    virtual JsonObject assignIconToScene( const SceneId& id, const Icon& icon ) = 0;
    virtual JsonObject addUserForNotification( const SceneId& id, const UserId& userId ) = 0;
    virtual JsonObject removeUserFromNotifications( const SceneId& id, const UserId& userId ) = 0;
    virtual JsonObject setSceneName( const SceneId& id, const Name& name ) = 0;
    virtual JsonObject setHouseModes( const SceneId& id, const HouseModes& houseModes ) = 0;
    virtual JsonObject setBlockEnabled( const SceneId& id, const BlockId& blockId, bool enabled ) = 0;

    virtual void actionBlockTest( const JsonObject& action, SendResponseCallback sendResponseCallback ) = 0;

    virtual bool setSceneBlocksSource( const std::string& sourceId ) = 0;
    virtual bool removeSceneBlocksSource( const std::string& sourceId ) = 0;
    virtual JsonObject sceneAllBlocksStatusReset( const SceneId& id ) = 0;
    virtual JsonObject sceneBlockStatusReset( const SceneId& id, const BlockId& blockId ) = 0;
    virtual JsonObject getSceneStatus( const SceneId& id ) = 0;
    virtual JsonObject getSceneState( const SceneId& id ) = 0;
    virtual JsonObject getBlockState( const SceneId& sceneId, const std::string& blockId ) = 0;

    virtual bool modifyScenes( const JsonArray& commands, const JsonArray& blockIds ) = 0;
    virtual JsonObject getExpressionValue( const std::string& name, const JsonObject& options ) = 0;

    virtual JsonObject getScenesInfo( const std::string& id ) = 0;

    virtual JsonObject upsertSceneExpression( const std::string& name,
                                              const JsonObject& params,
                                              const std::string& code,
                                              const std::string& valueType,
                                              const Json& metadata,
                                              bool variable,
                                              const JsonObject& value ) = 0;
    virtual JsonObject deleteSceneExpression( const std::string& name ) = 0;
    virtual JsonObject listSceneExpressions( bool showCode,
                                             bool strict,
                                             const std::vector< std::string >& filterTypes,
                                             bool hasVariable,
                                             bool variable,
                                             const std::vector< std::string >& names,
                                             bool showValue ) = 0;

    virtual JsonObject updateExpression( const std::string& name,
                                         const JsonObject& params,
                                         const std::string& code,
                                         const std::string& valueType,
                                         const JsonObject& metadata ) = 0;

    virtual JsonObject updateExpressionVariable( const std::string& name,
                                                 const JsonObject& value,
                                                 const std::string& valueType,
                                                 const JsonObject& metadata ) = 0;

    virtual JsonObject setMeta( const BaseRemoteInterface::SceneId& id, const BaseRemoteInterface::Meta& meta ) = 0;

    virtual JsonObject setBlockMeta( const BaseRemoteInterface::SceneId& id,
                                     const BaseRemoteInterface::BlockId& blockId,
                                     const BaseRemoteInterface::Meta& meta ) = 0;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
