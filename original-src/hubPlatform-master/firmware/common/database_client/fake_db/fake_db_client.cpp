#include "database_client/fake_db/fake_db_client.h"

#include "database_client/dbqueryparser.h"
#include "database_client/dbupdate.h"
#include "database_client/dbid.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_approximate_size.h"

#include <stdexcept>
#include <utility>

#include <unistd.h>

namespace HubPlatform {
namespace UT {

namespace {

class FakeDbClientError : public std::runtime_error {
public:
    explicit FakeDbClientError( const std::string& errDescription )
    : runtime_error( "FakeDbClient: " + errDescription ) {}
};

}  // namespace

FakeDbClient::~FakeDbClient() = default;

void FakeDbClient::insert( const std::string& dbName, const Json& json, const bool /*lockIgnore*/ ) {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    auto& namedDb = getNamedDb( dbName );
    auto obj = json;

    std::string key;
    if( !db::getId( obj, key ) ) {
        key = db::DbId::genOID();
    }

    if( obj.count( db::ID_FIELD_NAME ) == 0 ) {
        obj[ db::ID_FIELD_NAME ] = key;
    }

    /* real implementation of an IDbClient doesn't check if such a pair exists */
    namedDb[ key ] = std::move( obj );
}

void FakeDbClient::update( const std::string& dbName,
                           const Json& query,
                           const Json& obj,
                           bool upsert,
                           bool /*multi*/ ) {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    std::vector< Json > toUpsert;
    auto& namedDb = getNamedDb( dbName );

    auto handleItem = [&namedDb, &query, &obj]( const NamedDb::iterator& item ) {
        size_t arrayIndex = std::numeric_limits< size_t >::max();
        auto data = item->second;
        auto key = item->first;

        if( db::QueryParser::processing( data, query, arrayIndex ) ) {
            db::Update::processing( data, obj, arrayIndex );
            namedDb[ key ] = std::move( data );
        }
    };

    std::string id;
    if( db::getId( query, id ) ) {
        const auto it = namedDb.find( id );
        if( it != namedDb.end() )
            handleItem( it );
    }
    else {
        if( upsert )
            toUpsert.push_back( obj );
        else
            for( auto it = namedDb.begin(); it != namedDb.end(); it++ )
                handleItem( it );
    }

    if( upsert ) {
        multiInsert( dbName, toUpsert );
    }
}

void FakeDbClient::update( const std::string& dbName, const Json& query, std::function< void( Json& ) > updater ) {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    std::vector< Json > toUpsert;
    auto& namedDb = getNamedDb( dbName );

    auto handleItem = [&namedDb, &query, &updater]( const NamedDb::iterator& item ) {
        size_t arrayIndex = std::numeric_limits< size_t >::max();
        auto data = item->second;
        auto key = item->first;

        if( db::QueryParser::processing( data, query, arrayIndex ) ) {
            updater( data );
            namedDb[ key ] = std::move( data );
        }
    };

    std::string id;
    if( db::getId( query, id ) ) {
        const auto it = namedDb.find( id );
        if( it != namedDb.end() )
            handleItem( it );
    }
    else {
        for( auto it = namedDb.begin(); it != namedDb.end(); it++ )
            handleItem( it );
    }
}

void FakeDbClient::remove( const std::string& dbName, const Json& query, bool /*justOne*/, const bool /*lockIgnore*/ ) {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    auto& namedDb = getNamedDb( dbName );

    auto handleItem = [&namedDb, &query]( NamedDb::iterator& item ) {
        size_t arrayIndex = std::numeric_limits< size_t >::max();
        auto data = item->second;

        if( db::QueryParser::processing( data, query, arrayIndex ) )
            item = namedDb.erase( item );
        else
            item++;
    };

    std::string key;
    if( db::getId( query, key ) ) {
        auto it = namedDb.find( key );
        if( it != namedDb.end() )
            namedDb.erase( it ); /*real implementation of an IDbClient erases without checking other fields*/
    }
    else {
        auto it = namedDb.begin();
        while( it != namedDb.end() )
            handleItem( it );
    }
}

void FakeDbClient::capped( const std::string& dbName, int maxCollectionSize ) {}

bool FakeDbClient::exist( const std::string& dbName ) const {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    return m_dataBase.find( dbName ) != m_dataBase.end();
}

void FakeDbClient::create( const std::string& dbName ) {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    getNamedDb( dbName );
}

void FakeDbClient::clear( const std::string& dbName ) {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    getNamedDb( dbName ).clear();
}

void FakeDbClient::drop( const std::string& dbName ) {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    m_dataBase.erase( dbName );
}

void FakeDbClient::clean() {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    m_dataBase.clear();
}

void FakeDbClient::fsync() {}

void FakeDbClient::reset() {}

void FakeDbClient::lock() {}

void FakeDbClient::unlock() {}

void FakeDbClient::resetChangesCounter() {}

/** Returns a size of a table rounded to a system page size */
std::size_t FakeDbClient::size( const std::string& dbName ) const {
    auto foundIt = m_dataBase.find( dbName );
    if( foundIt != m_dataBase.cend() ) {
        const auto& table = foundIt->second;

        const int doubleQuotes = 2;
        uint64_t totalSize = 0;
        for( auto record = table.begin(); record != table.end(); record++ ) {
            auto keySize = record->first.size() + doubleQuotes;
            auto valueSize = jsonApproximateSize( record->second );

            totalSize += keySize + valueSize;
        }
        // return size multiple to a page size
        const auto pageSize = getpagesize();
        if( totalSize % pageSize != 0 )
            totalSize += pageSize - ( totalSize % pageSize );

        return totalSize;
    }
    return 0;
}

void FakeDbClient::connect( const int tries ) {}

bool FakeDbClient::connected() const {
    return {};
}

void FakeDbClient::fixBrokenPrimaryKeys( const std::string& dbName ) {}

std::vector< std::string > FakeDbClient::getPrimaryKeys( const std::string& dbName ) const {
    return {};
}

std::vector< std::string > FakeDbClient::getAllCollectionsNames( const bool /*onlyWithChanges*/ ) const {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );
    std::vector< NamedDbName > result;

    for( const auto& db : m_dataBase )
        result.push_back( db.first );

    return result;
}

void FakeDbClient::forEachNth( const std::string& dbName,
                               const Json& query,
                               const std::function< void( Json, const size_t ) >& func,
                               size_t nToReturn,
                               size_t nToSkip ) const {
    std::lock_guard< std::mutex > lock( m_dataBaseMtx );

    /* real implementation of an IDbClient creates a named db anyway, so we have no choice, but... */
    auto namedDb = const_cast< FakeDbClient& >( *this ).getNamedDb( dbName );

    auto handleItem = [&query, &func, &nToReturn]( const NamedDb::const_iterator& item ) {
        size_t arrayIndex = std::numeric_limits< size_t >::max();
        auto data = item->second;

        if( db::QueryParser::processing( data, query, arrayIndex ) ) {
            try {
                func( std::move( data ), arrayIndex );
            }
            catch( const std::exception& ex ) {
            }
            --nToReturn;
        }
    };

    std::string key;
    if( db::getId( query, key ) ) {
        auto it = namedDb.find( key );

        if( nToSkip == 0U && it != namedDb.end() )
            handleItem( it );
    }
    else {
        for( auto it = namedDb.begin(); it != namedDb.end() && nToReturn > 0; it++ ) {
            if( nToSkip ) {
                --nToSkip;
                continue;
            }

            handleItem( it );
        }
    }
}

FakeDbClient::NamedDb& FakeDbClient::getNamedDb( const NamedDbName& dbName ) {
    const auto* namedDb = checkNamedDb( dbName );
    if( namedDb != nullptr )
        return const_cast< NamedDb& >( *namedDb );

    auto result = m_dataBase.emplace( std::make_pair( dbName, NamedDb{} ) );
    if( !result.second )
        throw FakeDbClientError{ "fail to get a named db" };

    return result.first->second;
}

const FakeDbClient::NamedDb* FakeDbClient::checkNamedDb( const NamedDbName& dbName ) const {
    auto it = m_dataBase.find( dbName );
    if( it != m_dataBase.end() )
        return &( it->second );

    return nullptr;
}

/* TODO: I'm not sure what this function(look at the lmdb implementation) is for,
 *       but maybe it was added as a first step for some big task? */
void FakeDbClient::multiInsert( const std::string& dbName, const std::vector< Value >& jsonObjList ) {
    auto& namedDb = getNamedDb( dbName );

    for( auto elm : jsonObjList ) {
        std::string key;
        if( !db::getId( elm, key ) ) {
            key = db::DbId::genOID();
        }

        if( elm.count( db::ID_FIELD_NAME ) == 0 ) {
            elm[ db::ID_FIELD_NAME ] = key;
        }

        /* real implementation of an IDbClient doesn't check if such a pair exists */
        namedDb[ key ] = std::move( elm );
    }
}

}  // namespace UT
}  // namespace HubPlatform
