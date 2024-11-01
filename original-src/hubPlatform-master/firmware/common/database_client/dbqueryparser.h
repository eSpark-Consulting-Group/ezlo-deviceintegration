#pragma once

#include "json_utils/json_wrapper.h"

#include <string>

namespace db {

static const std::string ID_FIELD_NAME = "_id";

bool separateDottedName( const std::string& fieldName, std::string& mainFieldName, std::string& subFieldName );
bool getId( const Json& data, std::string& id );
bool copyField( const Json& data, Json& filteredResult, const std::string& fieldName );

class QueryParser {
public:
    static bool processing( const Json& data, const Json& query, size_t& arrayIndex );

private:
    enum class Operator : uint8_t {
        unknown,
        exists,
        OR,
        EQ,
        GT,
        GTE,
        LT,
        LTE,
        NE,
        IN,
        elemMatch,
    };
    static bool operatorCompare( const Json::value_type& dataValue,
                                 const Json::value_type& operatorValue,
                                 const Operator operation,
                                 size_t& arrayIndex );
    static bool valueCompare( const Json::value_type& dataValue,
                              const Json::value_type& operatorValue,
                              const Operator operation,
                              size_t& arrayIndex );
    static bool valuesCompare( const Json::value_type& dataValue,
                               const Json::value_type& queryValue,
                               size_t& arrayIndex );
    static bool compare( const Json& data,
                         const std::string& fieldName,
                         const Json::value_type& queryValue,
                         size_t& arrayIndex );
    static bool queryCheck( const Json& data, const Json::const_iterator& queryIt, size_t& arrayIndex );
    static Operator getOperator( const std::string& operatorName );
};

}  // namespace db
