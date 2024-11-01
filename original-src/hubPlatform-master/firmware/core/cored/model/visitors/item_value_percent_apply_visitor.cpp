#include "cored/model/visitors/item_value_percent_apply_visitor.h"
#include "cored/model/values/values.h"
#include "cored/model/exceptions.h"

#include "cored/model/visitors/item_value_float_visitor.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

using Value = CoreD::Model::Value;

namespace {

template < typename T >
double positionValueInMinMaxInterval( T const& currentValue, T const& minValue, T const& maxValue, T const& percent ) {
    return currentValue + ( maxValue - minValue ) * percent / 100.f;
}

double positionValueInMinMaxInterval( int currentValue, double minValue, double maxValue, double percent ) {
    return round( currentValue + ( maxValue - minValue ) * percent / 100.f );
}

template < typename Cur, typename T >
double newValueImpl( Cur const& currentValue, T const& minValue, T const& maxValue, T const& percent ) {
    double newValue = 0;

    if( percent > 100 ) {
        newValue = maxValue;
    }
    else if( percent < -100 ) {
        newValue = minValue;
    }
    else {
        newValue = positionValueInMinMaxInterval( currentValue, minValue, maxValue, percent );
        if( newValue > maxValue )
            newValue = maxValue;
        else if( newValue < minValue )
            newValue = minValue;
    }
    return newValue;
}

double valueToDouble( Value& value, ValueScale convertToScale = nullptr ) {
    ItemValueFloatVisitor toFloat( convertToScale );
    value.accept( toFloat );

    return toFloat.getValue();
}

}  // namespace

ItemValuePercentApplyVisitor::ItemValuePercentApplyVisitor( Value& minValue, Value& maxValue, double percent )
: m_minValue( minValue )
, m_maxValue( maxValue )
, m_percent( percent ) {}

void ItemValuePercentApplyVisitor::visit( Value& itemValue ) {
    throw UnexpectedValueType( itemValue.getType() );
}

void ItemValuePercentApplyVisitor::visit( IntValue& intValue ) {
    double minValue = valueToDouble( m_minValue );
    double maxValue = valueToDouble( m_maxValue );

    int newValue = newValueImpl( intValue.get(), minValue, maxValue, m_percent );
    intValue.set( newValue );
}

void ItemValuePercentApplyVisitor::visit( FloatValue& floatValue ) {
    double minValue = valueToDouble( m_minValue );
    double maxValue = valueToDouble( m_maxValue );

    auto newValue = newValueImpl( floatValue.get(), minValue, maxValue, m_percent );
    floatValue.set( newValue );
}

void ItemValuePercentApplyVisitor::visit( ScalableValue& itemValue ) {
    auto scale = itemValue.getScale();
    double minValue = valueToDouble( m_minValue, scale );
    double maxValue = valueToDouble( m_maxValue, scale );

    auto newValue = newValueImpl( static_cast< double >( itemValue.getValue() ), minValue, maxValue, m_percent );
    itemValue.setValue( newValue );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
