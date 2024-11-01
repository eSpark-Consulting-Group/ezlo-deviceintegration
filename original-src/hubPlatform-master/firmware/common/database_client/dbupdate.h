#ifndef DBUPDATE_H
#define DBUPDATE_H

#include "json_utils/json_wrapper.h"

#include <string>
#include <functional>

namespace db {

class Update {
public:
    static bool processing( Json& data, const Json& obj, const size_t arrayIndex );

private:
    typedef std::function< void( Json::value_type&, const Json::value_type& ) > UpdateAction;

    static size_t getIndex( const std::string& index, const size_t arrayIndex );
    static void checkArraySize( Json::value_type& array, const size_t i );
    static Json::value_type& getDotted( const std::string& fieldName,
                                        Json::value_type& field,
                                        const size_t arrayIndex );
    static Json::value_type& getValue( const std::string& fieldName, Json& data, const size_t arrayIndex );
    static bool updateEach( Json& data,
                            const Json::value_type& updatValue,
                            UpdateAction updateAction,
                            const size_t arrayIndex );
    static void setOperator( Json::value_type& value, const Json::value_type& updatValue );
    static void pushOperator( Json::value_type& value, const Json::value_type& updatValue );
    static void pullOperator( Json::value_type& value, const Json::value_type& updatValue );
    static void addToSetOperator( Json::value_type& value, const Json::value_type& updatValue );
    static void eachModifier( Json::value_type& value, const Json::value_type& updatValue, UpdateAction updateAction );
};

}  // namespace db

#endif  // DBUPDATE_H
