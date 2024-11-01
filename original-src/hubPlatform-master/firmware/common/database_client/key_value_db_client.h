#pragma once

#include "json_utils/json_fwd.h"

#include <string>
#include <utility>

class IDbClient;

namespace HubPlatform {
namespace Db {

/* @brief provides a key:value access to the db */
class KeyValueDbClient final {
public:
    using Key = std::string;
    using Bool = bool;
    using Number = double;
    using Int64 = int64_t;
    using String = std::string;
    using Json = nlohmann::json;

    template < typename T >
    using Result = std::pair< T, bool >;

    using GetBoolResult = Result< Bool >;
    using GetNumberResult = Result< Number >;
    using GetStringResult = Result< String >;
    using GetJsonResult = Result< Json >;

    /* @param[in] dbClient -
     * @param[in] dbName - name of a table to be wrapped by this
     * @param[in] dbMaxSize - limit in bytes, multiple to a page size */
    KeyValueDbClient( IDbClient& dbClient, std::string dbName, std::size_t dbMaxSize );

    GetBoolResult getBool( const Key& key ) const;
    GetNumberResult getNumber( const Key& key ) const;
    GetStringResult getString( const Key& key ) const;
    GetJsonResult getJson( const Key& key ) const;

    bool setBool( const Key& key, Bool value );
    bool setNumber( const Key& key, Number value );
    bool setInteger( const Key& key, Int64 value );
    bool setString( const Key& key, const String& value );
    bool setJson( const Key& key, const Json& value );

    bool exist( const Key& key ) const;
    void remove( const Key& key );
    void clear();

    /* @brief set a new limit (in bytes, multiple to a page size), the limit can only grow */
    bool setDbMaxSize( std::size_t dbMaxSize );

private:
    IDbClient& m_dbClient;
    std::string m_dbName;
    std::size_t m_dbMaxSize;
};

}  // namespace Db
}  // namespace HubPlatform
