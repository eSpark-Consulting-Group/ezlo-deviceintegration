#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"
#include <set>

namespace SceneEngine {

class DictionaryChangedEvent : public EventBase {
public:
    using Operation = std::string;
    using ItemId = std::string;
    using KeyValue = std::string;
    using Operations = std::set< Operation >;

    DictionaryChangedEvent( ItemId itemId, KeyValue keyValue, Operation operation );

    std::string getMethodName() const override;

    virtual bool compare( const KeyValue& keyValue, const Operation& operation ) const;

    const KeyValue& getKeyValue() const;

private:
    const Operation m_operation;
    KeyValue m_keyValue;
    ItemId m_itemId;
};

using DictionaryChangedEventPtr = std::shared_ptr< DictionaryChangedEvent >;

}  // namespace SceneEngine