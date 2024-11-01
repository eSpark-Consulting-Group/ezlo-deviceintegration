#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"

namespace SceneEngine {

class StringOperationEvent : public ItemEventBase {
public:
    StringOperationEvent( ItemValue itemValue, std::string operation );
    StringOperationEvent( ItemValueProviderPtr valueProvider, std::string operation );
    ~StringOperationEvent() override = default;

    std::string getMethodName() const override;

    ValueKeys getValueKeys() const override;

    bool compare( const ItemValue& itemValue ) const override;
    void checkValue( const ItemValue& leftValue ) const override;
    void setKeepState( bool value );

private:
    const ItemValue& getValue() const;

    static bool checkOperation( const ItemValue& left, const ItemValue& right, const std::string& operation );

    static void validateOperation( const std::string& operation );
    static void validateValue( const std::string& operation, const ItemValue& itemValue );

private:
    ItemValueProviderPtr m_valueProvider;

    ItemValue m_itemValue;
    std::string m_operation;

    mutable bool m_isTriggered{ false };
    bool m_keepState{ true };
};

using StringOperationEventPtr = std::shared_ptr< StringOperationEvent >;

}  // namespace SceneEngine
