#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include "json_utils/json_fwd.h"

#include <string>
#include <memory>

namespace SceneEngine {
class ItemEvent : public EventBase {
public:
    using ItemID = std::string;
    using DeviceID = std::string;
    using ItemValue = ItemValueWrapper;
    using Item = std::pair< ItemID, ItemValue >;

    enum class EventType { ITEM, DEVICE, EXPRESSION, ALL };
    enum class ArmedState { EMPTY, TRUE, FALSE };

    ItemEvent() = default;
    ItemEvent( const ItemEvent& ) = default;
    ItemEvent( ItemEvent&& ) = default;
    ItemEvent( const ItemID& id, ItemValueProviderPtr valueProvider );
    ItemEvent( const ItemID& itemId, ItemValueProviderPtr valueProvider, ArmedState deviceArmed );

    std::string getMethodName() const override;
    void setValueKey( const ValueSourceKey& valueKey );
    ValueKeys getValueKeys() const override;

    void setId( const ItemID& id );
    const ItemID& getId() const;

    const JsonObject& getValue() const;
    const std::string& getScale() const;
    void setScale( std::string scale );
    const ItemValueWrapper& getItemValue() const;

    void setEventType( EventType event );
    EventType getEventType() const;

    static ArmedState toArmedState( bool armed );

    bool compare( const ItemEvent& other ) const;
    bool compare( const ItemID& itemId,
                  const ItemValue& itemValue,
                  ArmedState deviceArmed,
                  bool isGroup = false ) const;
    bool compare( const ItemValue& itemValue, ArmedState deviceArmed, ItemEvent::EventType event ) const;

    void checkValue( const ItemValue& leftValue ) const;
    bool isDeviceUsed();

private:
    ItemValueProviderPtr m_valueProvider;
    std::unique_ptr< ValueSourceKey > m_valueKey;
    Item m_item;
    ArmedState m_deviceArmed{ ArmedState::EMPTY };
    EventType m_eventType{ EventType::ALL };
    std::string m_scale{};
};
using ItemEventPtr = std::shared_ptr< ItemEvent >;

}  // namespace SceneEngine
