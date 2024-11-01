#include "database_client/idbclient.h"

#include <limits>

void IDbClient::insert( const std::string& dbName, const Json& obj ) {
    insert( dbName, obj, false );
}

Json IDbClient::findOne( const std::string& dbName, const db::Query& query, const Json* fieldsToReturn ) const {
    auto result = findAll( dbName, query, fieldsToReturn, 1, 0 );
    return ( result.empty() ? Json::object() : std::move( result.front() ) );
}

std::vector< Json > IDbClient::findAll( const std::string& dbName,
                                        const db::Query& query,
                                        const Json* fieldsToReturn,
                                        int nToReturn,
                                        int nToSkip ) const {
    return findAll( dbName, query, fieldsToReturn, nToReturn, nToSkip, false );
}

std::vector< Json > IDbClient::findAll( const std::string& dbName,
                                        const db::Query& query,
                                        const Json* fieldsToReturn ) const {
    return findAll( dbName, query, fieldsToReturn, std::numeric_limits< int >::max(), 0 );
}

std::vector< Json > IDbClient::findAll( const std::string& dbName,
                                        const db::Query& query,
                                        const Json* fieldsToReturn,
                                        int nToReturn,
                                        int nToSkip,
                                        const bool /*lockIgnore*/ ) const {
    std::vector< Json > result;
    forEachNth( dbName,
                query.getFilter(),
                [&result, fieldsToReturn]( Json data, const size_t ) {
                    if( fieldsToReturn ) {
                        JsonObject dataToReturn = JSON_EMPTY_OBJ;
                        for( auto it = fieldsToReturn->cbegin(); it != fieldsToReturn->cend(); ++it ) {
                            const auto dataIt = data.find( it.key() );
                            if( dataIt != data.end() ) {
                                const auto newKey =
                                    it.value().is_string() ? it.value().get< std::string >() : dataIt.key();
                                dataToReturn[ newKey ] = std::move( dataIt.value() );
                            }
                        }
                        result.push_back( std::move( dataToReturn ) );
                    }
                    else {
                        result.push_back( std::move( data ) );
                    }
                },
                ( nToReturn == 0 ) ? ( std::numeric_limits< size_t >::max )() : nToReturn,
                nToSkip );

    return result;
}

size_t IDbClient::count( const std::string& dbName, const Json& query ) const {
    size_t result = 0U;
    forEachNth(
        dbName, query, [&result]( Json, const size_t ) { ++result; }, std::numeric_limits< size_t >::max(), 0U );

    return result;
}

bool IDbClient::has( const std::string& dbName, const Json& query ) const {
    bool result = false;
    forEachNth( dbName, query, [&result]( Json, const size_t ) { result = true; }, 1, 0U );

    return result;
}

void IDbClient::remove( const std::string& dbName, const Json& query ) {
    remove( dbName, query, false, false );
}

void IDbClient::removeAll( const std::set< std::string >& excluded ) {
    for( const std::string& dbName : getDatabaseNames() )
        if( excluded.count( dbName ) == 0 )
            for( const std::string& collectionName : getCollectionNames( dbName ) )
                remove( collectionName, JSON_EMPTY_OBJ, false, true );
}

std::vector< std::string > IDbClient::getAllNames() const {
    return getAllNames( {}, false );
}

std::string IDbClient::dumpCollection( const std::string& dbName, int indentSpacesAmount ) const {
    return Json( findAll( dbName, db::Query(), nullptr, 0, 0, true ) ).dump( indentSpacesAmount );
}

bool IDbClient::restoreCollection( const std::string& dbName, const std::string& collection ) {
    Json jsonData;
    try {
        jsonData = Json::parse( collection );
    }
    catch( ... ) {
        return false;
    }
    if( !jsonData.is_array() )
        return false;
    remove( dbName, JSON_EMPTY_OBJ, false, true );
    for( const auto& element : jsonData )
        insert( dbName, element, true );
    return true;
}

std::vector< std::string > IDbClient::getAllNames( const std::set< std::string >& excludedDb,
                                                   const bool onlyWithChanges ) const {
    return getCollectionsNamesByCondition( onlyWithChanges, [&excludedDb]( const std::string& collectionName ) {
        return !excludedDb.count( collectionName );
    } );
}

std::vector< std::string > IDbClient::getDatabaseNames() const {
    auto result = getAllCollectionsNames( false );

    for( std::string& collectionName : result ) {
        collectionName.erase( std::find( collectionName.begin(), collectionName.end(), '.' ), collectionName.end() );
    }

    std::sort( result.begin(), result.end() );
    result.erase( std::unique( result.begin(), result.end() ), result.end() );

    return result;
}

std::vector< std::string > IDbClient::getCollectionNames( const std::string& dbName ) const {
    return getCollectionsNamesByCondition( false, [&dbName]( const std::string& collectionName ) {
        return collectionName.compare( 0u, collectionName.find( '.' ), dbName ) == 0;
    } );
}

template < typename Condition >
std::vector< std::string > IDbClient::getCollectionsNamesByCondition( const bool onlyWithChanges,
                                                                      Condition&& condition ) const {
    auto result = getAllCollectionsNames( onlyWithChanges );
    result.erase(
        std::remove_if( result.begin(),
                        result.end(),
                        [&condition]( const std::string& collectionName ) { return !condition( collectionName ); } ),
        result.end() );

    return result;
}
