#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include "scenes_manager/scenes_engine/events/item_event_base_factory.h"
#include <memory>

namespace SceneEngine {

class SceneException;

class ItemStateChangedEvent : public ItemEventBase {
public:
    ItemStateChangedEvent( ItemValueProviderPtr startValueProvider, ItemValueProviderPtr finishValueProvider );
    ItemStateChangedEvent( ItemStateChangedEvent&& ) = default;
    ~ItemStateChangedEvent() override = default;

    std::string getMethodName() const override;

    ValueKeys getValueKeys() const override;

    bool compare( const ItemValue& itemValue ) const override;

    bool checkChangeExpected( const ItemValueWrapper& oldValue, const ItemValueWrapper& newValue ) const;

private:
    static const ItemValueWrapper* fetchValue( ItemValueProviderInterface& valueProvider );

    static void checkValuesComparability( const ItemValueWrapper* startValue,
                                          const ItemValueWrapper* finishValue,
                                          const ItemValueWrapper& oldValue,
                                          const ItemValueWrapper& newValue );

    static SceneException createValuesIncomparableException( const char* paramName,
                                                             const std::string& paramValueType,
                                                             const char* otherValueName,
                                                             const std::string& otherValueType );

private:
    ItemValueProviderPtr m_startValueProvider;
    ItemValueProviderPtr m_finishValueProvider;
    mutable bool m_startConditionSatisfied{ false };
    mutable ItemValueWrapper m_oldValue;
};

using ItemStateChangedEventPtr = std::shared_ptr< ItemStateChangedEvent >;

}  // namespace SceneEngine
