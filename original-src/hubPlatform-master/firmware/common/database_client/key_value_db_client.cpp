#include "database_client/key_value_db_client.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_approximate_size.h"
#include <unistd.h>
#include <utility>

namespace HubPlatform {
namespace Db {

namespace {

template < typename T >
KeyValueDbClient::Result< T > get( const std::string dbName,
                                   const IDbClient& dbClient,
                                   const KeyValueDbClient::Key& key ) {
    const auto fields = JSON( "value"
                              << "value" );
    const auto result = dbClient.findOne( dbName, JSON( db::ID_FIELD_NAME << key ), &fields );
    if( result.is_object() && !result.empty() ) {
        T value;

        if( json_get( result, "value", value ) )
            return { value, true };
    }

    return { {}, false };
}

template < typename T >
bool set( const std::string dbName,
          IDbClient& dbClient,
          const KeyValueDbClient::Key& key,
          const T& value,
          size_t dbMaxSize ) {
    dbClient.insert( dbName, JSON( db::ID_FIELD_NAME << key << "value" << value ) );

    /* the easiest way to control a db's table (named database) size */
    if( dbClient.size( dbName ) > dbMaxSize ) {
        dbClient.remove( dbName, JSON( db::ID_FIELD_NAME << key ) );
        return false;
    }
    return true;
}

}  // namespace

KeyValueDbClient::KeyValueDbClient( IDbClient& dbClient, std::string dbName, std::size_t dbMaxSize )
: m_dbClient( dbClient )
, m_dbName( std::move( dbName ) )
, m_dbMaxSize( dbMaxSize < getpagesize() ? getpagesize() : dbMaxSize ) {}

KeyValueDbClient::GetBoolResult KeyValueDbClient::getBool( const Key& key ) const {
    return get< Bool >( m_dbName, m_dbClient, key );
}

KeyValueDbClient::GetNumberResult KeyValueDbClient::getNumber( const Key& key ) const {
    return get< Number >( m_dbName, m_dbClient, key );
}

KeyValueDbClient::GetStringResult KeyValueDbClient::getString( const Key& key ) const {
    return get< String >( m_dbName, m_dbClient, key );
}

KeyValueDbClient::GetJsonResult KeyValueDbClient::getJson( const Key& key ) const {
    return get< Json >( m_dbName, m_dbClient, key );
}

bool KeyValueDbClient::setBool( const Key& key, Bool value ) {
    return set< Bool >( m_dbName, m_dbClient, key, value, m_dbMaxSize );
}

bool KeyValueDbClient::setNumber( const Key& key, Number value ) {
    return set< Number >( m_dbName, m_dbClient, key, value, m_dbMaxSize );
}

bool KeyValueDbClient::setInteger( const Key& key, Int64 value ) {
    return set< Int64 >( m_dbName, m_dbClient, key, value, m_dbMaxSize );
}

bool KeyValueDbClient::setString( const Key& key, const String& value ) {
    if( m_dbClient.size( m_dbName ) + key.size() + value.size() + 4 > m_dbMaxSize )
        return false;
    return set< String >( m_dbName, m_dbClient, key, value, m_dbMaxSize );
}

bool KeyValueDbClient::setJson( const Key& key, const Json& value ) {
    size_t approximateSize = jsonApproximateSize( value );
    if( m_dbClient.size( m_dbName ) + key.size() + approximateSize > m_dbMaxSize )
        return false;
    return set< Json >( m_dbName, m_dbClient, key, value, m_dbMaxSize );
}

bool KeyValueDbClient::exist( const Key& key ) const {
    const auto result = m_dbClient.findOne( m_dbName, JSON( db::ID_FIELD_NAME << key ), nullptr );
    if( result.is_object() && !result.empty() )
        return true;

    return false;
}

void KeyValueDbClient::remove( const Key& key ) {
    m_dbClient.remove( m_dbName, JSON( db::ID_FIELD_NAME << key ) );
}

void KeyValueDbClient::clear() {
    m_dbClient.remove( m_dbName, JSON_EMPTY_OBJ );
}

bool KeyValueDbClient::setDbMaxSize( std::size_t dbMaxSize ) {
    if( dbMaxSize >= m_dbMaxSize ) {
        m_dbMaxSize = dbMaxSize;
        return true;
    }

    return false;
}

}  // namespace Db
}  // namespace HubPlatform
