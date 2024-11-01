#include "scenes_manager/scenes_engine/events/compare_number_range_event.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

CompareNumberRangeEvent::CompareNumberRangeEvent( ItemValue startValue, ItemValue endValue, std::string comparator )
: m_startValue( std::move( startValue ) )
, m_endValue( std::move( endValue ) )
, m_comparator( std::move( comparator ) ) {}

std::string CompareNumberRangeEvent::getMethodName() const {
    return Model::Resources::MethodNames::compareNumberRange;
}

bool CompareNumberRangeEvent::compare( const ItemValue& value ) const {
    if( m_comparator.empty() || m_comparator == Model::Resources::ValueComparators::Between ) {
        return value >= m_startValue && value <= m_endValue;
    }
    else if( m_comparator == Model::Resources::ValueComparators::NotBetween ) {
        return value < m_startValue || value > m_endValue;
    }

    return false;
}

}  // namespace SceneEngine
