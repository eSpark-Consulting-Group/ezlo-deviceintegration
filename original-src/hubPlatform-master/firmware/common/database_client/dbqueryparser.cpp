#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"

#include <tiny/tiny_consolecolors.h>

#include <unordered_map>

namespace db {

bool QueryParser::processing( const Json& data, const Json& query, size_t& arrayIndex ) {
    if( query.is_object() )
        for( auto it = query.cbegin(); it != query.cend(); ++it )
            if( !queryCheck( data, it, arrayIndex ) )
                return false;
    return true;
}

bool QueryParser::operatorCompare( const Json::value_type& dataValue,
                                   const Json::value_type& operatorValue,
                                   const Operator operation,
                                   size_t& arrayIndex ) {
    switch( operation ) {
    case Operator::GT:
        return ( dataValue > operatorValue );
    case Operator::GTE:
        return ( dataValue >= operatorValue );
    case Operator::LT:
        return ( dataValue < operatorValue );
    case Operator::LTE:
        return ( dataValue <= operatorValue );
    case Operator::EQ:
        return ( dataValue == operatorValue );
    case Operator::NE:
        return ( dataValue != operatorValue );
    case Operator::IN:
        if( operatorValue.is_array() ) {
            for( const auto& value : operatorValue )
                if( dataValue == value )
                    return true;
        }
        return false;
    case Operator::exists:
        return true;
    case Operator::elemMatch:
        if( dataValue.is_object() )
            return processing( dataValue, operatorValue, arrayIndex );
        return valuesCompare( dataValue, operatorValue, arrayIndex );
    case Operator::unknown:
    default:
        return false;
    }
}

bool QueryParser::valueCompare( const Json::value_type& dataValue,
                                const Json::value_type& operatorValue,
                                const QueryParser::Operator operation,
                                size_t& arrayIndex ) {
    if( dataValue.is_string() && operatorValue.is_string() ) {
        const std::string str1 = dataValue.get< std::string >();
        const std::string str2 = operatorValue.get< std::string >();
        return operatorCompare( str1.compare( str2 ), 0L, operation, arrayIndex );
    }
    return operatorCompare( dataValue, operatorValue, operation, arrayIndex );
}

bool QueryParser::valuesCompare( const Json::value_type& dataValue,
                                 const Json::value_type& queryValue,
                                 size_t& arrayIndex ) {
    if( queryValue.type() == Json::value_t::object ) {
        bool hasOperators = false;
        for( auto it = queryValue.cbegin(); it != queryValue.cend(); ++it ) {
            const std::string& queryOperator = it.key();
            if( queryOperator.front() == '$' ) {
                hasOperators = true;
                if( !valueCompare( dataValue, it.value(), getOperator( queryOperator ), arrayIndex ) )
                    return false;
            }
        }
        if( hasOperators )
            return true;
    }
    return ( dataValue == queryValue );
}

bool QueryParser::compare( const Json& data,
                           const std::string& fieldName,
                           const Json::value_type& queryValue,
                           size_t& arrayIndex ) {
    std::string mainFieldName, newFieldName;
    if( !separateDottedName( fieldName, mainFieldName, newFieldName ) ) {
        if( data.count( fieldName ) < 1 )
            return false;
        const Json::value_type& dataValue = data[ fieldName ];
        if( dataValue.type() == Json::value_t::array ) {
            if( dataValue.empty() && queryValue.empty() ) {
                return true;
            }
            size_t i = 0;
            for( const auto& value : dataValue ) {
                if( valuesCompare( value, queryValue, arrayIndex ) ) {
                    arrayIndex = i;
                    return true;
                }
                ++i;
            }
            return false;
        }
        return valuesCompare( dataValue, queryValue, arrayIndex );
    }
    if( data.count( mainFieldName ) < 1 )
        return false;
    const Json::value_type& dataValue = data[ mainFieldName ];
    size_t i = 0;
    switch( dataValue.type() ) {
    case Json::value_t::object:
        return compare( dataValue, newFieldName, queryValue, arrayIndex );
    case Json::value_t::array:
        if( dataValue.empty() && queryValue.empty() ) {
            return true;
        }
        for( const auto& value : dataValue ) {
            if( value.type() == Json::value_t::object )
                if( compare( value, newFieldName, queryValue, arrayIndex ) ) {
                    arrayIndex = i;
                    return true;
                }
            ++i;
        }
        return false;
    default:
        return false;
    }
}

bool QueryParser::queryCheck( const Json& data, const Json::const_iterator& queryIt, size_t& arrayIndex ) {
    const std::string fieldName = queryIt.key();
    const Json::value_type& queryValue = queryIt.value();
    if( fieldName.front() == '$' )
        switch( getOperator( fieldName ) ) {
        case Operator::OR:
            if( queryValue.type() != Json::value_t::array ) {
                Log::error( cc::error( "ERROR: query processing: operator $or should be array, but it's: " ) +
                            cc::warn( queryValue.dump() ) );
                return false;
            }
            for( const auto& orQuery : queryValue )
                if( orQuery.type() != Json::value_t::object ) {
                    Log::error(
                        cc::error( "ERROR: query processing: operator $or should contain objects, but it has: " ) +
                        cc::warn( orQuery.dump() ) );
                    return false;
                }
                else {
                    if( processing( data, orQuery, arrayIndex ) )
                        return true;
                }
            return false;
        case Operator::unknown:
        default:
            return false;
        }
    if( queryValue.type() == Json::value_t::object )
        for( auto it = queryValue.cbegin(); it != queryValue.cend(); ++it ) {
            const std::string& queryOperator = it.key();
            if( queryOperator.front() == '$' && getOperator( queryOperator ) == Operator::exists &&
                !getBool( it.value() ) )
                return ( !compare( data, fieldName, queryValue, arrayIndex ) );
        }
    return compare( data, fieldName, queryValue, arrayIndex );
}

QueryParser::Operator QueryParser::getOperator( const std::string& operatorName ) {
    static const std::unordered_map< std::string, Operator > operators = {
        { "$exists", Operator::exists }, { "$or", Operator::OR },
        { "$eq", Operator::EQ },         { "$gt", Operator::GT },
        { "$gte", Operator::GTE },       { "$lt", Operator::LT },
        { "$lte", Operator::LTE },       { "$ne", Operator::NE },
        { "$in", Operator::IN },         { "$elemMatch", Operator::elemMatch },
    };
    try {
        return operators.at( operatorName );
    }
    catch( ... ) {
        Log::error( cc::error( "ERROR: query processing: unknown operator: " ) + cc::warn( operatorName ) );
        return Operator::unknown;
    }
}

bool getId( const Json& data, std::string& id ) {
    try {
        if( data.count( ID_FIELD_NAME ) == 0 )
            return false;
        const Json& val = data.at( ID_FIELD_NAME );
        id = val.get< std::string >();
        return true;
    }
    catch( ... ) {
    }
    return false;
}

bool separateDottedName( const std::string& fieldName, std::string& mainFieldName, std::string& subFieldName ) {
    size_t dotPosition = fieldName.find( '.' );
    if( dotPosition == std::string::npos )
        return false;
    mainFieldName = fieldName.substr( 0, dotPosition );
    subFieldName = fieldName.substr( dotPosition + 1 );
    return true;
}

bool copyArray( const Json::value_type& data, Json::value_type& filteredResult, const std::string& fieldName ) {
    if( !filteredResult.is_array() )
        filteredResult = Json::array();
    size_t i = 0;
    bool result = false;
    for( const auto& element : data ) {
        if( element.is_object() ) {
            while( filteredResult.size() <= i )
                filteredResult.push_back( Json() );
            if( copyField( element, filteredResult[ i ], fieldName ) )
                result = true;
        }
        ++i;
    }
    return result;
}

bool copyField( const Json& data, Json& filteredResult, const std::string& fieldName ) {
    std::string mainFieldName, subFieldName;
    if( separateDottedName( fieldName, mainFieldName, subFieldName ) ) {
        if( data.count( mainFieldName ) ) {
            Json tmpBuff;
            if( filteredResult.count( mainFieldName ) )
                tmpBuff = filteredResult.at( mainFieldName );
            bool isCopyOk = false;
            if( data.at( mainFieldName ).is_object() )
                isCopyOk = copyField( data.at( mainFieldName ), tmpBuff, subFieldName );
            else if( data.at( mainFieldName ).is_array() )
                isCopyOk = copyArray( data.at( mainFieldName ), tmpBuff, subFieldName );
            if( isCopyOk ) {
                filteredResult[ mainFieldName ] = std::move( tmpBuff );
                return true;
            }
        }
    }
    else if( data.count( fieldName ) ) {
        filteredResult[ fieldName ] = data.at( fieldName );
        return true;
    }
    return false;
}

}  // namespace db
