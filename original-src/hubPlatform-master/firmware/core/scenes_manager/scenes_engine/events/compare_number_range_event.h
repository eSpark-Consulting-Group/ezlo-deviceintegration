#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"

namespace SceneEngine {

class CompareNumberRangeEvent : public ItemEventBase {
public:
    using ItemID = std::string;
    using ItemEventBase::ItemValue;
    using Comparator = std::function< bool( const ItemValue&, const ItemValue& ) >;

    CompareNumberRangeEvent( ItemValue startValue, ItemValue endValue, std::string comparator );

    std::string getMethodName() const override;

    bool compare( const ItemValue& value ) const override;

private:
    ItemValue m_startValue;
    ItemValue m_endValue;
    std::string m_comparator;
};

using CompareNumberRangeEventPtr = std::shared_ptr< CompareNumberRangeEvent >;

}  // namespace SceneEngine
