#include "scenes_manager/scenes_engine/events/item_event_base_factory.h"
#include "scenes_manager/scenes_engine/events/compare_numbers_event.h"
#include "scenes_manager/scenes_engine/events/compare_number_range_event.h"
#include "scenes_manager/scenes_engine/events/button_event.h"
#include "scenes_manager/scenes_engine/events/user_lock_event.h"
#include "scenes_manager/scenes_engine/events/compare_strings_event.h"
#include "scenes_manager/scenes_engine/events/string_operation_event.h"
#include "scenes_manager/scenes_engine/events/in_array_event.h"
#include "scenes_manager/scenes_engine/events/compare_values_event.h"

namespace SceneEngine {

ItemEventBasePtr ItemEventBaseFactory::makeCompareNumbers( const ItemValueWrapper& itemValue,
                                                           const ItemValueComparator& compareFunction ) {
    return std::make_shared< CompareNumbersEvent >( itemValue, compareFunction );
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareNumbers( ItemValueProviderPtr valueProvider,
                                                           const Comparator& compareFunction ) {
    return std::make_shared< CompareNumbersEvent >( std::move( valueProvider ), compareFunction );
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareNumbersStateless( const ItemValueWrapper& itemValue,
                                                                    const ItemValueComparator& compareFunction ) {
    auto event = std::make_shared< CompareNumbersEvent >( itemValue, compareFunction );
    event->setKeepState( false );
    return event;
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareNumbersStateless( ItemValueProviderPtr valueProvider,
                                                                    const Comparator& compareFunction ) {
    auto event = std::make_shared< CompareNumbersEvent >( std::move( valueProvider ), compareFunction );
    event->setKeepState( false );
    return event;
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareNumberRange( const ItemValueWrapper& startValue,
                                                               const ItemValueWrapper& endValue,
                                                               const std::string& comparator ) {
    return std::make_shared< CompareNumberRangeEvent >( startValue, endValue, comparator );
}

ItemEventBasePtr ItemEventBaseFactory::makeButtonState( const std::string& buttonState ) {
    return std::make_shared< ButtonStateEvent >( buttonState );
}

ItemEventBasePtr ItemEventBaseFactory::makeUserLock( const std::string& itemId,
                                                     const std::string& userActionType,
                                                     int userIndex ) {
    return std::make_shared< UserLockEvent >( itemId, userActionType, userIndex );
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareStrings( const ItemValueWrapper& itemValue,
                                                           const ItemValueComparator& compareFunction ) {
    return std::make_shared< CompareStringsEvent >( itemValue, compareFunction );
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareStrings( ItemValueProviderPtr valueProvider,
                                                           const Comparator& compareFunction ) {
    return std::make_shared< CompareStringsEvent >( std::move( valueProvider ), compareFunction );
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareStringsStateless( const ItemValueWrapper& itemValue,
                                                                    const ItemValueComparator& compareFunction ) {
    auto event = std::make_shared< CompareStringsEvent >( itemValue, compareFunction );
    event->setKeepState( false );
    return event;
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareStringsStateless( ItemValueProviderPtr valueProvider,
                                                                    const Comparator& compareFunction ) {
    auto event = std::make_shared< CompareStringsEvent >( std::move( valueProvider ), compareFunction );
    event->setKeepState( false );
    return event;
}

ItemEventBasePtr ItemEventBaseFactory::makeStringOperation( const ItemValueWrapper& itemValue,
                                                            const std::string& operation ) {
    return std::make_shared< StringOperationEvent >( itemValue, operation );
}

ItemEventBasePtr ItemEventBaseFactory::makeStringOperation( ItemValueProviderPtr itemValueProvider,
                                                            const std::string& operation ) {
    return std::make_shared< StringOperationEvent >( std::move( itemValueProvider ), operation );
}

ItemEventBasePtr ItemEventBaseFactory::makeStringOperationStateless( const ItemValueWrapper& itemValue,
                                                                     const std::string& operation ) {
    auto event = std::make_shared< StringOperationEvent >( itemValue, operation );
    event->setKeepState( false );
    return event;
}

ItemEventBasePtr ItemEventBaseFactory::makeStringOperationStateless( ItemValueProviderPtr valueProvider,
                                                                     const std::string& operation ) {
    auto event = std::make_shared< StringOperationEvent >( std::move( valueProvider ), operation );
    event->setKeepState( false );
    return event;
}

ItemEventBasePtr ItemEventBaseFactory::makeInArray( std::vector< ItemValueWrapper > valuesArray,
                                                    const std::string& operation ) {
    return std::make_shared< InArrayEvent >( std::move( valuesArray ), operation );
}

ItemEventBasePtr ItemEventBaseFactory::makeInArrayStateless( std::vector< ItemValueWrapper > valuesArray,
                                                             const std::string& operation ) {
    auto event = std::make_shared< InArrayEvent >( std::move( valuesArray ), operation );
    event->setKeepState( false );
    return event;
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareValues( const ItemValueWrapper& itemValue,
                                                          std::string comparator,
                                                          std::string valueType ) {
    return std::make_shared< CompareValuesEvent >( itemValue, std::move( comparator ), std::move( valueType ) );
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareValues( ItemValueProviderPtr valueProvider,
                                                          std::string comparator,
                                                          std::string valueType ) {
    return std::make_shared< CompareValuesEvent >(
        std::move( valueProvider ), std::move( comparator ), std::move( valueType ) );
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareValuesStateless( const ItemValueWrapper& itemValue,
                                                                   std::string comparator,
                                                                   std::string valueType ) {
    auto event = std::make_shared< CompareValuesEvent >( itemValue, std::move( comparator ), std::move( valueType ) );
    event->setKeepState( false );
    return event;
}

ItemEventBasePtr ItemEventBaseFactory::makeCompareValuesStateless( ItemValueProviderPtr valueProvider,
                                                                   std::string comparator,
                                                                   std::string valueType ) {
    auto event = std::make_shared< CompareValuesEvent >(
        std::move( valueProvider ), std::move( comparator ), std::move( valueType ) );
    event->setKeepState( false );
    return event;
}

}  // namespace SceneEngine
