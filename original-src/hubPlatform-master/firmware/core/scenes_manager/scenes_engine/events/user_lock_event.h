#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"

namespace SceneEngine {

class UserLockEvent : public ItemEventBase {
public:
    using ItemID = std::string;
    using ItemEventBase::ItemValue;
    using LockActionType = std::string;
    using UserId = int;

    UserLockEvent( const ItemID& itemId, const LockActionType& actionType, UserId userId );
    UserLockEvent( ItemID&& itemId, LockActionType&& actionType, UserId userId );

    std::string getMethodName() const override;

    bool compare( const ItemID& itemId, const ItemValue& itemValue ) const;
    bool compare( const ItemID& itemId ) const;
    bool compare( const ItemValue& value ) const override;

    const ItemID& getItemId() const;

private:
    ItemID m_itemId;
    LockActionType m_lockActionType;
    UserId m_userId;
};

using UserLockEventPtr = std::shared_ptr< UserLockEvent >;

}  // namespace SceneEngine
