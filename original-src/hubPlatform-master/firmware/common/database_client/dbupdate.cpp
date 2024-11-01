#include "database_client/dbupdate.h"
#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"

#include <tiny/tiny_consolecolors.h>

#include <unordered_map>

namespace db {

bool Update::processing( Json& data, const Json& obj, const size_t arrayIndex ) {
    static const std::unordered_map< std::string, UpdateAction > updateActions = {
        { "$set", setOperator },
        { "$push",
          std::bind( eachModifier,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     pushOperator ) },  // l_sergiy: TO-FIX: growing unlimited array?
        { "$pull", pullOperator },
        { "$addToSet", std::bind( eachModifier, std::placeholders::_1, std::placeholders::_2, addToSetOperator ) },
    };
    // Json dataCopy = data;
    if( !obj.empty() && obj.cbegin().key().front() == '$' ) {
        const std::string& updateOperator = obj.cbegin().key();
        const Json::value_type& updatValue = obj.cbegin().value();
        UpdateAction updateAction;
        try {
            updateAction = updateActions.at( updateOperator );
        }
        catch( ... ) {
            Log::error( cc::error( "ERROR: unknown update operator: " ) + cc::warn( updateOperator ) );
            return false;
        }
        return updateEach( data, updatValue, updateAction, arrayIndex );
    }
    std::string id;
    getId( data, id );
    data = obj;
    data[ ID_FIELD_NAME ] = id;
    return true;  //(dataCopy != data);
}

size_t Update::getIndex( const std::string& index, const size_t arrayIndex ) {
    size_t i = 0;
    if( index == "$" ) {
        if( arrayIndex != std::string::npos )
            i = arrayIndex;
        if( i > 10 )
            i = 0;
    }
    else
        try {
            i = std::stoi( index );
        }
        catch( ... ) {
            return std::string::npos;
        }
    return i;
}

void Update::checkArraySize( Json::value_type& array, const size_t i ) {
    if( i > array.size() ) {
        size_t addElements = i - array.size();
        while( addElements-- )
            array.push_back( nullptr );
    }
}

Json::value_type& Update::getDotted( const std::string& fieldName, Json::value_type& field, const size_t arrayIndex ) {
    switch( field.type() ) {
    case Json::value_t::array: {
        std::string subFieldName;
        std::string index = fieldName;
        bool hasDot = separateDottedName( fieldName, index, subFieldName );
        size_t i = getIndex( index, arrayIndex );
        if( i == std::string::npos ) {
            i = 0;
            for( auto& element : field ) {
                if( element.type() == Json::value_t::object )
                    if( element.count( index ) )
                        return ( hasDot ? getDotted( subFieldName, element[ index ], arrayIndex ) : element[ index ] );
            }
        }
        checkArraySize( field, i );
        return ( hasDot ? getDotted( subFieldName, field[ i ], arrayIndex ) : field[ i ] );
    }
    case Json::value_t::object:
        break;
    default:
        field = Json();
    }
    return getValue( fieldName, field, arrayIndex );
}

Json::value_type& Update::getValue( const std::string& fieldName, Json& data, const size_t arrayIndex ) {
    std::string mainFieldName, subFieldName;
    if( separateDottedName( fieldName, mainFieldName, subFieldName ) )
        return getDotted( subFieldName, data[ mainFieldName ], arrayIndex );
    return data[ fieldName ];
}

bool Update::updateEach( Json& data,
                         const Json::value_type& updatValue,
                         Update::UpdateAction updateAction,
                         const size_t arrayIndex ) {
    for( auto it = updatValue.cbegin(); it != updatValue.cend(); ++it ) {
        const std::string& fieldName = it.key();
        if( fieldName != ID_FIELD_NAME )
            updateAction( getValue( fieldName, data, arrayIndex ), it.value() );
    }
    return true;
}

void Update::setOperator( Json::value_type& value, const Json::value_type& updatValue ) {
    value = updatValue;
}

void Update::pushOperator( Json::value_type& value, const Json::value_type& updatValue ) {
    if( !value.is_array() && value.empty() )
        value = JSON_EMPTY_ARR;
    if( value.type() == Json::value_t::array )  // else error wrong type
        value.push_back( updatValue );
}

void Update::pullOperator( Json::value_type& value, const Json::value_type& updatValue ) {
    size_t arrayIndex = std::string::npos;
    if( value.is_array() )
        for( auto it = value.cbegin(); it != value.cend(); )
            it = ( it.value() == updatValue ||
                   ( it.value().type() == Json::value_t::object && updatValue.type() == Json::value_t::object &&
                     QueryParser::processing( it.value(), updatValue, arrayIndex ) ) ) ?
                     value.erase( it ) :
                     it + 1;
}

void Update::addToSetOperator( Json::value_type& value, const Json::value_type& updatValue ) {
    if( !value.is_array() ) {
        if( value.empty() )
            value = JSON_EMPTY_ARR;
        else
            return;  // error wrong type
    }
    for( auto it = value.cbegin(); it != value.cend(); ++it )
        if( it.value() == updatValue )
            return;
    value.push_back( updatValue );
}

void Update::eachModifier( Json::value_type& value, const Json::value_type& updatValue, UpdateAction updateAction ) {
    static const std::string each = "$each";
    if( updatValue.type() == Json::value_t::object && updatValue.count( each ) ) {
        const Json::value_type& eachArray = updatValue.at( each );
        if( eachArray.type() == Json::value_t::array )
            for( const auto& eachValue : eachArray )
                updateAction( value, eachValue );
        else
            Log::error( cc::error( "ERROR: update processing: $each modifier should be array: " ) +
                        cc::warn( eachArray.dump() ) );
    }
    else
        updateAction( value, updatValue );
}

}  // namespace db
