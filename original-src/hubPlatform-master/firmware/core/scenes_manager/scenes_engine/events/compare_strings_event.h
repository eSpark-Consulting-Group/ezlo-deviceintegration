#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"

namespace SceneEngine {

class CompareStringsEvent : public ItemEventBase {
public:
    using Comparator = std::function< bool( const ItemValue&, const ItemValue& ) >;

    CompareStringsEvent( ItemValue itemValue, Comparator comparator );
    CompareStringsEvent( ItemValueProviderPtr valueProvider, Comparator comparator );

    template < typename T >
    CompareStringsEvent( const T& itemValue, Comparator comparator )
    : m_itemValue( Json{ { "value", itemValue } } )
    , m_comparator( std::move( comparator ) ) {}

    void checkValue( const ItemValue& leftValue ) const override;
    bool compare( const ItemValue& value ) const override;
    void setKeepState( bool value );

    std::string getMethodName() const override;

    ValueKeys getValueKeys() const override;
    const ItemValue& getValue() const;

private:
    ItemValueProviderPtr m_valueProvider;
    ItemValue m_itemValue;
    Comparator m_comparator;
    mutable bool m_isTriggered{ false };
    bool m_keepState{ true };
};

using CompareStringsEventPtr = std::shared_ptr< CompareStringsEvent >;

}  // namespace SceneEngine
