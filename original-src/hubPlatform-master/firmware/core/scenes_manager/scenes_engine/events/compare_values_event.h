#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include <utility>

namespace SceneEngine {

class CompareValuesEvent : public ItemEventBase {
public:
    CompareValuesEvent( ItemValue itemValue, std::string comparator, std::string valueType );
    CompareValuesEvent( ItemValueProviderPtr valueProvider, std::string comparator, std::string valueType );

    bool compare( const ItemValue& value ) const override;

    void checkValue( const ItemValue& value ) const override;
    void setKeepState( bool value );

    std::string getMethodName() const override;

    ValueKeys getValueKeys() const override;
    const ItemValue& getValue() const;
    const std::string& getValueType() const;

private:
    using ComparatorMethod = std::function< bool( const ItemValue&, const ItemValue& ) >;
    void checkEmptyValueType() const;
    void checkComparator() const;

private:
    ItemValueProviderPtr m_valueProvider;
    ItemValue m_itemValue;
    std::string m_comparator;
    const std::string m_valueType;
    mutable ComparatorMethod m_comparatorMethod;
    mutable bool m_isTriggered{ false };
    bool m_keepState{ true };
};

using CompareValuesEventPtr = std::shared_ptr< CompareValuesEvent >;

}  // namespace SceneEngine
