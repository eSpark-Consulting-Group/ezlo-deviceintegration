
#include "database_client/lmdb_client.h"
#include "database_client/dbid.h"
#include "database_client/dbqueryparser.h"
#include "database_client/dbupdate.h"
#include "database_client/dbtools.h"

#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"
#include "database_client/db_tables_names.h"
#include "system_utils/environment.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"

#include <cassert>

namespace {
std::string dbRefToString( std::reference_wrapper< const std::pair< const char*, size_t > > valueRef ) {
    const auto str = valueRef.get().first;
    const auto size = valueRef.get().second;
    const auto hasZero = size && str[ size - 1 ] == '\0';
    return std::string( str, size - hasZero );
}
std::pair< const char*, size_t > stringToDbRef( std::reference_wrapper< const std::string > str ) {
    return { str.get().c_str(), str.get().size() + 1 };
}
}  // namespace

LMDBClient::LMDBClient( const std::string& pathToDB, const std::string& pathToTemporaryDB )
: m_env( lmdb::Env( pathToDB, MDB_NOSUBDIR, TableNamesSize, HubPlatform::DatabaseClient::getStorageMapSizeDefault() ) )
, m_tempEnv( lmdb::Env( pathToTemporaryDB,
                        MDB_NOSUBDIR,
                        TableNamesSize,
                        HubPlatform::DatabaseClient::getTempStorageMapSizeDefault() ) ) {}

LMDBClient::LMDBClient()
: LMDBClient( getStorageFilePath(), getTemporaryStorageFilePath() ) {}

void LMDBClient::insert( const std::string& dbName, const Json& obj, const bool /*lockIgnore*/ ) {
    std::string id;
    if( !db::getId( obj, id ) ) {
        id = db::DbId::genOID();
    }

    auto record = obj;
    if( record.count( db::ID_FIELD_NAME ) == 0 ) {
        record[ db::ID_FIELD_NAME ] = id;
    }

    auto& env = getEnvInstanceForTableName( dbName );
    auto txn = env.createTransaction();
    auto dbi = env.open< const char, const char >( txn, dbName.c_str(), MDB_CREATE );
    const auto data = record.dump();
    dbi.put( txn, { stringToDbRef( id ), stringToDbRef( data ) } );
    txn.commit();
}

void LMDBClient::multiInsert( const std::string& dbName, const std::vector< Json >& list ) {
    auto& env = getEnvInstanceForTableName( dbName );
    auto txn = env.createTransaction();
    auto dbi = env.open< const char, const char >( txn, dbName.c_str(), MDB_CREATE );
    {
        for( const auto& elem : list ) {
            std::string id;
            if( !db::getId( elem, id ) ) {
                id = db::DbId::genOID();
            }
            auto record = elem;
            if( record.count( db::ID_FIELD_NAME ) == 0 ) {
                record[ db::ID_FIELD_NAME ] = id;
            }
            const auto data = record.dump();
            dbi.put( txn, { stringToDbRef( id ), stringToDbRef( data ) } );
        }
        txn.commit();
    }
}

void LMDBClient::getAllCollectionsNames( const lmdb::Env& env, std::vector< std::string >& names ) const {
    auto txn = env.createTransaction();
    const auto& dbi = env.open< const char, const char >( txn, nullptr, MDB_CREATE );
    {
        // Note: cursor should be closed before transaction commit.
        auto cursor = dbi.createCursor( txn );
        while( cursor.seek( MDB_NEXT ) ) {
            auto item = cursor.get();
            names.push_back( dbRefToString( item.key ) );
        }
    }
    txn.commit();
}

void LMDBClient::update( const std::string& dbName, const Json& query, const Json& obj, bool upsert, bool /*multi*/ ) {
    std::vector< Json > toUpsert;
    {
        auto& env = getEnvInstanceForTableName( dbName );
        auto txn = env.createTransaction();
        auto dbi = env.open< const char, const char >( txn, dbName.data(), MDB_CREATE );
        {
            // Note: cursor should be closed before transaction commit.
            auto cursor = dbi.createCursor( txn );

            auto handleItem = [&cursor, &query, &obj]() {
                auto item = cursor.get();
                auto data = Json::parse( dbRefToString( item.value ) );
                size_t arrayIndex = std::numeric_limits< size_t >::max();
                if( db::QueryParser::processing( data, query, arrayIndex ) ) {
                    db::Update::processing( data, obj, arrayIndex );

                    // TODO: handle changed "_id"
                    const auto key = dbRefToString( item.key );
                    const auto value = data.dump();
                    cursor.put( { stringToDbRef( key ), stringToDbRef( value ) }, MDB_CURRENT );
                }
            };

            std::string id;
            if( db::getId( query, id ) ) {
                if( cursor.seek( stringToDbRef( id ), MDB_SET ) ) {
                    handleItem();
                }
            }
            else {
                if( upsert ) {
                    toUpsert.push_back( obj );
                }
                else {
                    while( cursor.seek( MDB_NEXT ) ) {
                        handleItem();
                    }
                }
            }
        }
        txn.commit();
    }

    if( upsert ) {
        multiInsert( dbName, toUpsert );
    }
}

void LMDBClient::update( const std::string& dbName, const Json& query, std::function< void( Json& ) > updater ) {
    {
        auto& env = getEnvInstanceForTableName( dbName );
        auto txn = env.createTransaction();
        auto dbi = env.open< const char, const char >( txn, dbName.data(), MDB_CREATE );
        {
            // Note: cursor should be closed before transaction commit.
            auto cursor = dbi.createCursor( txn );

            auto handleItem = [&cursor, &query, &updater]() {
                auto item = cursor.get();
                auto data = Json::parse( dbRefToString( item.value ) );
                size_t arrayIndex = std::numeric_limits< size_t >::max();
                if( db::QueryParser::processing( data, query, arrayIndex ) ) {
                    const auto key = dbRefToString( item.key );
                    updater( data );

                    const auto value = data.dump();

                    // TODO: handle changed "_id"
                    cursor.put( { stringToDbRef( key ), stringToDbRef( value ) }, MDB_CURRENT );
                }
            };

            std::string id;
            if( db::getId( query, id ) ) {
                if( cursor.seek( stringToDbRef( id ), MDB_SET ) ) {
                    handleItem();
                }
            }
            else {
                while( cursor.seek( MDB_NEXT ) ) {
                    handleItem();
                }
            }
        }
        txn.commit();
    }
}

void LMDBClient::fixBrokenPrimaryKeys( const std::string& dbName ) {
    LOGGER_DEBUG( "LMDBClient::fixBrokenPrimaryKeys " + dbName );

    auto& env = getEnvInstanceForTableName( dbName );
    auto txn = env.createTransaction();
    auto dbi = env.open< const char, const char >( txn, dbName.data(), MDB_CREATE );

    // We have to use O(N^2) algorithm, explanation:
    // - There're cases of broken primary keys with broken hashes, so we can't store keys and delete all of them on next
    // iteration
    // - If we erase broken items, an elements order can be potentially changed ( I'm not exactly sure, but it's more
    // safe to consider order changing )

    for( bool fixed = false; !fixed; ) {
        fixed = true;
        auto cursor = dbi.createCursor( txn );

        while( fixed && cursor.seek( MDB_NEXT ) ) {
            const auto item = cursor.get();
            const auto record = Json::parse( dbRefToString( item.value ) );
            const auto prevKey = dbRefToString( item.key );
            const auto& idJson = record[ db::ID_FIELD_NAME ];
            if( idJson.is_string() &&
                ( idJson != prevKey || idJson.get< const std::string* >()->size() + 1 != item.key.second ) ) {
                cursor.erase();

                const std::string key = record[ db::ID_FIELD_NAME ];
                const auto data = record.dump();
                cursor.put( { stringToDbRef( key ), stringToDbRef( data ) } );
                fixed = false;

                LOGGER_INFO( "Fix broken PK: '" + prevKey + "' -> '" + key + "'" );
            }
        }
    }

    txn.commit();
}

std::vector< std::string > LMDBClient::getPrimaryKeys( const std::string& dbName ) const {
    std::vector< std::string > result;
    auto& env = getEnvInstanceForTableName( dbName );
    auto txn = env.createTransaction();
    const auto& dbi = env.open< const char, const char >( txn, dbName.data(), MDB_CREATE );
    {
        auto cursor = dbi.createCursor( txn );

        while( cursor.seek( MDB_NEXT ) ) {
            const auto item = cursor.get();
            result.push_back( dbRefToString( item.key ) );
        }
    }

    txn.commit();

    return result;
}

void LMDBClient::remove( const std::string& dbName, const Json& query, bool /*justOne*/, const bool /*lockIgnore*/ ) {
    auto& env = getEnvInstanceForTableName( dbName );
    auto txn = env.createTransaction();
    auto dbi = env.open< const char, const char >( txn, dbName.data(), MDB_CREATE );
    {
        // Note: cursor should be closed before transaction commit.
        auto cursor = dbi.createCursor( txn );

        std::string id;
        if( db::getId( query, id ) ) {
            if( cursor.seek( stringToDbRef( id ), MDB_SET ) ) {
                cursor.erase();
            }
        }
        else {
            while( cursor.seek( MDB_NEXT ) ) {
                auto item = cursor.get();
                auto data = Json::parse( dbRefToString( item.value ) );
                size_t arrayIndex = std::numeric_limits< size_t >::max();
                if( db::QueryParser::processing( data, query, arrayIndex ) ) {
                    cursor.erase();
                }
            }
        }
    }
    txn.commit();
}

void LMDBClient::capped( const std::string& /*dbName*/, int /*maxCollectionSize*/ ) {
    // TODO: Implement if needed.
}

bool LMDBClient::exist( const std::string& dbName ) const {
    auto& env = getEnvInstanceForTableName( dbName );

    auto txn = env.createTransaction();
    auto result = env.exist( txn, dbName.c_str() );
    txn.commit();
    return result;
}

void LMDBClient::create( const std::string& dbName ) {
    auto& env = getEnvInstanceForTableName( dbName );

    auto txn = env.createTransaction();
    env.open< const char, const char >( txn, dbName.c_str(), MDB_CREATE );
    txn.commit();
}

void LMDBClient::clear( const std::string& dbName ) {
    auto& env = getEnvInstanceForTableName( dbName );

    auto txn = env.createTransaction();
    if( !env.exist( txn, dbName.c_str() ) ) {
        return;
    }
    auto dbi = env.open< const char, const char >( txn, dbName.c_str(), 0 );
    dbi.clear( txn );
    txn.commit();
}

void LMDBClient::drop( const std::string& dbName ) {
    auto& env = getEnvInstanceForTableName( dbName );

    auto txn = env.createTransaction();
    if( !env.exist( txn, dbName.c_str() ) ) {
        return;
    }
    auto dbi = env.open< const char, const char >( txn, dbName.c_str(), 0 );
    dbi.drop( txn );
    txn.commit();
}

void LMDBClient::clean() {
    cleanInstance( m_env );
    cleanInstance( m_tempEnv );
}

void LMDBClient::fsync() {
    // TODO: Implement if needed.
}

void LMDBClient::reset() {
    // do nothing.
}

void LMDBClient::lock() {
    // do nothing.
}

void LMDBClient::unlock() {
    // do nothing.
}

void LMDBClient::resetChangesCounter() {
    // TODO: Implement if needed.
}

std::size_t LMDBClient::size( const std::string& dbName ) const {
    const auto& env = getEnvInstanceForTableName( dbName );

    auto txn = env.createTransaction();
    const auto& dbi = env.open< const char, const char >( txn, dbName.c_str(), MDB_CREATE );
    const auto dbStat = dbi.getStat( txn );

    return dbStat.ms_psize * ( dbStat.ms_leaf_pages + dbStat.ms_branch_pages + dbStat.ms_overflow_pages );
}

void LMDBClient::connect( const int /*tries*/ ) {
    // do nothing.
}

bool LMDBClient::connected() const {
    return true;
}

const std::string LMDBClient::createTemporaryTableName( const std::string& tableName ) {
    std::string res = TemporaryTableNamePreffix;
    res += tableName;
    return std::move( res );
}

std::vector< std::string > LMDBClient::getPermanentCollectionsNames() const {
    std::vector< std::string > result;
    getAllCollectionsNames( m_env, result );
    return result;
}

std::vector< std::string > LMDBClient::getTemporaryCollectionsNames() const {
    std::vector< std::string > result;
    getAllCollectionsNames( m_tempEnv, result );
    return result;
}

std::vector< std::string > LMDBClient::getAllCollectionsNames( const bool /*onlyWithChanges*/ ) const {
    // Note: In current implementation it does not support changes handling.
    // Investigate: do we need this functionality?
    std::vector< std::string > result;
    getAllCollectionsNames( m_env, result );
    getAllCollectionsNames( m_tempEnv, result );
    return result;
}

void LMDBClient::forEachNth( const std::string& dbName,
                             const Json& query,
                             const std::function< void( Json, const size_t ) >& func,
                             size_t nToReturn,
                             size_t nToSkip ) const {
    const auto& env = getEnvInstanceForTableName( dbName );
    auto txn = env.createTransaction();
    const auto& dbi = env.open< const char, const char >( txn, dbName.data(), MDB_CREATE );
    {
        // Note: cursor should be closed before transaction commit.
        auto cursor = dbi.createCursor( txn );

        auto handleItem = [&cursor, &query, &func, &nToReturn]() {
            const auto item = cursor.get();
            auto data = Json::parse( dbRefToString( item.value ) );
            size_t arrayIndex = std::numeric_limits< size_t >::max();
            if( db::QueryParser::processing( data, query, arrayIndex ) ) {
                try {
                    func( std::move( data ), arrayIndex );
                }
                catch( const std::exception& ex ) {
                }
                nToReturn--;
            }
        };

        std::string id;
        if( db::getId( query, id ) ) {
            if( nToSkip == 0U && cursor.seek( stringToDbRef( id ), MDB_SET ) ) {
                handleItem();
            }
        }
        else {
            while( cursor.seek( MDB_NEXT ) && nToReturn > 0 ) {
                if( nToSkip ) {
                    --nToSkip;
                    continue;
                }
                handleItem();
            }
        }
    }
    txn.commit();
}

lmdb::Env& LMDBClient::getEnvInstanceForTableName( const std::string& tableName ) {
    return tableName.find( TemporaryTableNamePreffix ) == 0 ? m_tempEnv : m_env;
}

const lmdb::Env& LMDBClient::getEnvInstanceForTableName( const std::string& tableName ) const {
    return tableName.find( TemporaryTableNamePreffix ) == 0 ? m_tempEnv : m_env;
}

void LMDBClient::cleanInstance( lmdb::Env& env ) {
    {
        auto txn = env.createTransaction();
        auto rootDBI = env.open< const char, const char >( txn, nullptr, MDB_CREATE );

        // Note: cursor should be closed before transaction commit.
        {
            auto cursor = rootDBI.createCursor( txn );
            while( cursor.seek( MDB_NEXT ) ) {
                auto item = cursor.get();
                auto dbi = env.open< const char, const char >( txn, item.key.first, MDB_CREATE );
                dbi.clear( txn );
            }
        }

        txn.commit();
    }
    {
        auto txn = env.createTransaction();
        auto rootDBI = env.open< const char, const char >( txn, nullptr, MDB_CREATE );

        rootDBI.clear( txn );

        txn.commit();
    }
}

std::unique_ptr< IDbClient > createDbClient() {
    return createDbClient( getStorageFilePath(), getTemporaryStorageFilePath() );
}

std::unique_ptr< IDbClient > createDbClient( std::string&& dbPath, std::string&& temporaryDbPath ) {
    const std::string errMessage{ "Failed to read database path from config" };
    try {
        if( dbPath.empty() ) {
            HubPlatform::ConfigReader configReader;
            if( configReader.hasValue( HubPlatform::ConfigKeys::dbPathKey ) ) {
                dbPath = configReader.getString( HubPlatform::ConfigKeys::dbPathKey );
            }
        }
    }
    catch( const std::exception& ex ) {
        std::cout << errMessage << ": " << ex.what() << std::endl;
    }
    catch( ... ) {
        std::cout << errMessage << std::endl;
    }

    return std::make_unique< LMDBClient >( std::move( dbPath ), std::move( temporaryDbPath ) );
}
