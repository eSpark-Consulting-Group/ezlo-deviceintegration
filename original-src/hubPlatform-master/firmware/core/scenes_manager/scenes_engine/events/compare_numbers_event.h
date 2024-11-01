#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"

namespace SceneEngine {

class CompareNumbersEvent : public ItemEventBase {
public:
    using Comparator = std::function< bool( const ItemValue&, const ItemValue& ) >;

    CompareNumbersEvent( ItemValue itemValue, Comparator comparator );
    CompareNumbersEvent( ItemValueProviderPtr valueProvider, Comparator comparator );

    template < typename T >
    CompareNumbersEvent( const T& itemValue, Comparator comparator )
    : m_itemValue( Json{ { "value", itemValue } } )
    , m_comparator( std::move( comparator ) ) {}

    bool compare( const ItemValue& value ) const override;

    void checkValue( const ItemValue& value ) const override;

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

using CompareNumbersEventPtr = std::shared_ptr< CompareNumbersEvent >;

}  // namespace SceneEngine
