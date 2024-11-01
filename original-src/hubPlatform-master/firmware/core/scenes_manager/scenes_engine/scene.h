#pragma once

#include "json_utils/json_fwd.h"
#include <string>
#include <memory>
#include <set>

namespace SceneEngine {
namespace UT {
class SceneTest;
}

class Scene final {
public:
    using SceneID = std::string;
    using ParentId = std::string;
    using GroupId = std::string;
    using Name = std::string;
    using RoomName = std::string;
    using Tag = std::string;
    using UserId = std::string;
    using HouseMode = std::string;
    using UserNotifications = std::set< UserId >;
    using HouseModes = std::set< HouseMode >;
    using Icon = std::string;
    using Error = std::string;
    using Meta = JsonObject;
    using GlobalRestrictionId = std::string;
    using Generator = JsonObject;

    explicit Scene( SceneID id );

    Scene( const Scene& ) = delete;
    Scene( Scene&& ) = delete;
    Scene& operator=( const Scene& ) = delete;
    Scene& operator=( Scene&& ) = delete;

    void setEnabled( bool enabled );
    void setGroupEnabled( bool enabled );
    void setGlobal( bool global );
    void setParentId( const ParentId& parentId );
    void setRoomName( const RoomName& roomName );
    void setGroupId( const GroupId& groupId );
    void setName( const Name& name );
    void setTag( const Tag& tag );
    void setIcon( const Icon& icon );
    void setHouseModes( const HouseModes& houseModes );
    void setErrorData( Json error );
    void setMeta( const JsonObject& meta );
    void setGlobalRestrictionId( const GlobalRestrictionId& id );
    void setGenerator( const Generator& data );

    const SceneID& getId() const;
    const ParentId& getParentId() const;
    const GroupId& getGroupId() const;
    const Name& getName() const;
    const RoomName& getRoomName() const;
    const Tag& getTag() const;
    const HouseModes& getHouseModes() const;
    const UserNotifications& getUserNotifications() const;
    const Icon& getIcon() const;
    const Json& getErrorData() const;
    const Meta& getMeta() const;
    const GlobalRestrictionId& getGlobalRestrictionId() const;
    const Generator& getGeneratorData() const;

    bool isEnabled() const;
    bool isGroup() const;
    bool isGlobal() const;

    bool addUserNotification( const UserId& userId );
    bool removeUserNotification( const UserId& userId );
    bool addHouseMode( const HouseMode& houseMode );
    bool removeHouseMode( const HouseMode& houseMode );

private:
    SceneID m_id;
    ParentId m_parentId;
    RoomName m_roomName;
    GroupId m_groupId;
    Name m_name;
    Meta m_meta;
    Tag m_tag;
    bool m_isEnabled{ true };
    bool m_isGroup{ false };
    bool m_isGlobal{ false };
    UserNotifications m_userNotifications;
    HouseModes m_houseModes;
    Icon m_icon;
    Json m_errorObject;
    GlobalRestrictionId m_globalRestrictionId;
    Generator m_generator;
};

using SceneSharedPtr = std::shared_ptr< Scene >;

}  // namespace SceneEngine
