#include "scenes_manager/scenes_engine/events/user_lock_event.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

UserLockEvent::UserLockEvent( const ItemID& itemId, const LockActionType& actionType, UserId userId )
: m_itemId( itemId )
, m_lockActionType( actionType )
, m_userId( userId ) {}

UserLockEvent::UserLockEvent( ItemID&& itemId, LockActionType&& actionType, UserId userId )
: m_itemId( std::move( itemId ) )
, m_lockActionType( std::move( actionType ) )
, m_userId( userId ) {}

std::string UserLockEvent::getMethodName() const {
    return Model::Resources::MethodNames::isUserLockOperation;
}

bool UserLockEvent::compare( const ItemID& itemId, const ItemValue& itemValue ) const {
    return compare( itemId ) && compare( itemValue );
}

bool UserLockEvent::compare( const ItemID& itemId ) const {
    return m_itemId == itemId;
}

bool UserLockEvent::compare( const ItemValue& value ) const {
    using namespace HubPlatform::CoreD::Model::Resources;
    auto action = json_get_str( value.getValue(), UserLockOperation::Fields::Action );
    auto userId = json_get_int( value.getValue(), UserLockOperation::Fields::UserId );
    return m_lockActionType == action && m_userId == userId;
}

const UserLockEvent::ItemID& UserLockEvent::getItemId() const {
    return m_itemId;
}

}  // namespace SceneEngine
