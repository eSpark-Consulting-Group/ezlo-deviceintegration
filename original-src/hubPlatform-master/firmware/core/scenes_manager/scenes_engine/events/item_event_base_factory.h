#pragma once
#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"

namespace SceneEngine {

class ItemEventBaseFactory {
public:
    using Comparator = ItemValueComparator;

public:
    static ItemEventBasePtr makeCompareNumbers( const ItemValueWrapper& itemValue,
                                                const ItemValueComparator& compareFunction );

    static ItemEventBasePtr makeCompareNumbers( ItemValueProviderPtr valueProvider, const Comparator& compareFunction );

    static ItemEventBasePtr makeCompareNumbersStateless( const ItemValueWrapper& itemValue,
                                                         const ItemValueComparator& compareFunction );

    static ItemEventBasePtr makeCompareNumbersStateless( ItemValueProviderPtr valueProvider,
                                                         const Comparator& compareFunction );

    static ItemEventBasePtr makeCompareNumberRange( const ItemValueWrapper& startValue,
                                                    const ItemValueWrapper& endValue,
                                                    const std::string& comparator );
    static ItemEventBasePtr makeButtonState( const std::string& buttonState );

    static ItemEventBasePtr makeUserLock( const std::string& itemId, const std::string& userActionType, int userIndex );

    static ItemEventBasePtr makeCompareStrings( const ItemValueWrapper& itemValue,
                                                const ItemValueComparator& compareFunction );

    static ItemEventBasePtr makeCompareStrings( ItemValueProviderPtr valueProvider, const Comparator& compareFunction );

    static ItemEventBasePtr makeCompareStringsStateless( const ItemValueWrapper& itemValue,
                                                         const ItemValueComparator& compareFunction );

    static ItemEventBasePtr makeCompareStringsStateless( ItemValueProviderPtr valueProvider,
                                                         const Comparator& compareFunction );

    static ItemEventBasePtr makeStringOperation( const ItemValueWrapper& itemValue, const std::string& operation );

    static ItemEventBasePtr makeStringOperation( ItemValueProviderPtr itemValueProvider, const std::string& operation );

    static ItemEventBasePtr makeInArray( std::vector< ItemValueWrapper > valuesArray, const std::string& operation );

    static ItemEventBasePtr makeCompareValues( const ItemValueWrapper& itemValue,
                                               std::string comparator,
                                               std::string valueType );

    static ItemEventBasePtr makeCompareValues( ItemValueProviderPtr valueProvider,
                                               std::string comparator,
                                               std::string valueType );

    static ItemEventBasePtr makeCompareValuesStateless( const ItemValueWrapper& itemValue,
                                                        std::string comparator,
                                                        std::string valueType );

    static ItemEventBasePtr makeCompareValuesStateless( ItemValueProviderPtr valueProvider,
                                                        std::string comparator,
                                                        std::string valueType );
    static ItemEventBasePtr makeStringOperationStateless( const ItemValueWrapper& itemValue,
                                                          const std::string& operation );

    static ItemEventBasePtr makeStringOperationStateless( ItemValueProviderPtr valueProvider,
                                                          const std::string& operation );

    static ItemEventBasePtr makeInArrayStateless( std::vector< ItemValueWrapper > valuesArray,
                                                  const std::string& operation );
};

}  // namespace SceneEngine
