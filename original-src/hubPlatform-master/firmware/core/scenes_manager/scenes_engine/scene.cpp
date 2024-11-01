#include "scenes_manager/scenes_engine/scene.h"
#include "json_utils/json_fwd.h"
#include <utility>

namespace SceneEngine {

Scene::Scene( Scene::SceneID id )
: m_id( std::move( id ) ) {}

void Scene::setEnabled( bool enabled ) {
    m_isEnabled = enabled;
}

void Scene::setGroupEnabled( bool enabled ) {
    m_isGroup = enabled;
}

void Scene::setGlobal( bool global ) {
    m_isGlobal = global;
}

void Scene::setGroupId( const Scene::GroupId& groupId ) {
    m_groupId = groupId;
}

void Scene::setTag( const Scene::Tag& tag ) {
    m_tag = tag;
}

void Scene::setIcon( const Scene::Icon& icon ) {
    m_icon = icon;
}

void Scene::setHouseModes( const Scene::HouseModes& houseModes ) {
    m_houseModes = houseModes;
}

void Scene::setParentId( const std::string& parentId ) {
    m_parentId = parentId;
}

void Scene::setRoomName( const std::string& roomName ) {
    m_roomName = roomName;
}

void Scene::setName( const std::string& name ) {
    m_name = name;
}

void Scene::setMeta( const JsonObject& meta ) {
    m_meta = meta;
}

const Scene::SceneID& Scene::getId() const {
    return m_id;
}

const Scene::ParentId& Scene::getParentId() const {
    return m_parentId;
}

const Scene::GroupId& Scene::getGroupId() const {
    return m_groupId;
}

const Scene::Name& Scene::getName() const {
    return m_name;
}

const Scene::RoomName& Scene::getRoomName() const {
    return m_roomName;
}

const Scene::Tag& Scene::getTag() const {
    return m_tag;
}

const Scene::HouseModes& Scene::getHouseModes() const {
    return m_houseModes;
}

const Scene::UserNotifications& Scene::getUserNotifications() const {
    return m_userNotifications;
}

const Scene::Icon& Scene::getIcon() const {
    return m_icon;
}

const Scene::Meta& Scene::getMeta() const {
    return m_meta;
}

bool Scene::isEnabled() const {
    return m_isEnabled;
}

bool Scene::isGroup() const {
    return m_isGroup;
}

bool Scene::isGlobal() const {
    return m_isGlobal;
}

bool Scene::addUserNotification( const Scene::UserId& userId ) {
    return m_userNotifications.insert( userId ).second;
}

bool Scene::removeUserNotification( const Scene::UserId& userId ) {
    return m_userNotifications.erase( userId ) != 0;
}

bool Scene::addHouseMode( const Scene::HouseMode& houseMode ) {
    return m_houseModes.insert( houseMode ).second;
}

bool Scene::removeHouseMode( const Scene::HouseMode& houseMode ) {
    return m_houseModes.erase( houseMode ) != 0;
}

void Scene::setErrorData( Json error ) {
    m_errorObject = std::move( error );
}

const Json& Scene::getErrorData() const {
    return m_errorObject;
}

void Scene::setGlobalRestrictionId( const Scene::GlobalRestrictionId& id ) {
    m_globalRestrictionId = id;
}

const Scene::GlobalRestrictionId& Scene::getGlobalRestrictionId() const {
    return m_globalRestrictionId;
}

void Scene::setGenerator( const Scene::Generator& data ) {
    m_generator = data;
}

const Scene::Generator& Scene::getGeneratorData() const {
    return m_generator;
}

}  // namespace SceneEngine
