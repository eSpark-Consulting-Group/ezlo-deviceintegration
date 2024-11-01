#pragma once

#include <lmdb/lmdb.h>

#include "config_reader/config_reader.h"
#include <cstring>
#include <memory>
#include <type_traits>
#include <config_reader/config_keys.h>

namespace lmdb {

class Error : public std::runtime_error {
public:
    explicit Error( int mdbErrorCode )
    : std::runtime_error(::mdb_strerror( mdbErrorCode ) )
    , m_errorCode( mdbErrorCode ) {}
    using std::runtime_error::runtime_error;

    int errorCode() const {
        return m_errorCode;
    }

private:
    int m_errorCode = { MDB_SUCCESS };
};

class PanicError final : public Error {
public:
    explicit PanicError( int mdbErrorCode )
    : Error( mdbErrorCode ) {}
};

class MapFullError final : public Error {
public:
    explicit MapFullError( int mdbErrorCode )
    : Error( mdbErrorCode ) {}
};

class EnvMapResizedError final : public Error {
public:
    explicit EnvMapResizedError( int mdbErrorCode )
    : Error( mdbErrorCode ) {}
};

inline void throwIfError( int mdbErrorCode ) {
    if( 0 == mdbErrorCode )
        return;

    switch( mdbErrorCode ) {
    case MDB_MAP_RESIZED:
        throw EnvMapResizedError( mdbErrorCode );
    case MDB_PANIC:
        throw PanicError( mdbErrorCode );
    case MDB_MAP_FULL:
        throw MapFullError( mdbErrorCode );
    default:
        throw Error( mdbErrorCode );
    }
}

class Env;

template < typename Key, typename Value >
class DataBase;

class Transaction final {
    friend class Env;

    template < typename Key, typename Value >
    friend class DataBase;

public:
    void commit() {
        if( const auto error = ::mdb_txn_commit( _txn.get() ) ) {
            throwIfError( error );
        }
        _txn.release();
    }

private:
    explicit Transaction( MDB_txn* txn )
    : _txn( txn, ::mdb_txn_abort ) {}

    operator MDB_txn*() const noexcept {
        return _txn.get();
    }

private:
    std::unique_ptr< MDB_txn, void ( * )( MDB_txn* ) > _txn;
};

template < typename Key, typename Value >
class DataBase final {
    friend class Env;

public:
    struct Item final {
        static_assert( std::is_pod< Key >::value, "Key should be a POD type" );
        static_assert( std::is_pod< Value >::value, "Value should be a POD type" );

        using KeyRef = std::pair< const Key*, size_t >;
        using ValueRef = std::pair< const Value*, size_t >;

        Item( const KeyRef& k, const ValueRef& v )
        : key( k )
        , value( v ) {}

        const KeyRef key;
        const ValueRef value;
    };

    class ConstCursor {
        friend class DataBase;

    public:
        bool seek( const typename Item::KeyRef& key, MDB_cursor_op op ) {
            auto mdbKey = createVal( key );
            MDB_val value{};

            const auto error = ::mdb_cursor_get( *this, &mdbKey, &value, op );
            switch( error ) {
            case MDB_SUCCESS:
                return true;

            case MDB_NOTFOUND:
                return false;

            default:
                break;
            }
            throwIfError( error );
            return false;
        }

        bool seek( MDB_cursor_op op ) {
            return seek( { nullptr, 0 }, op );
        }

        Item get() const {
            MDB_val key{};
            MDB_val value{};
            if( const auto error = ::mdb_cursor_get( *this, &key, &value, MDB_GET_CURRENT ) ) {
                throwIfError( error );
            }
            return Item( { static_cast< Key* >( key.mv_data ), key.mv_size },
                         { static_cast< Value* >( value.mv_data ), value.mv_size } );
        }

        ConstCursor( const ConstCursor& ) = delete;
        ConstCursor( ConstCursor&& ) noexcept = default;

    protected:
        ConstCursor( MDB_cursor* const cursor )
        : _cursor( cursor, ::mdb_cursor_close ) {}

        template < typename Val >
        static MDB_val createVal( const std::pair< const Val*, size_t >& orig ) {
            return { orig.second, (void*) orig.first };
        }

        operator MDB_cursor*() const noexcept {
            return _cursor.get();
        }

    private:
        std::unique_ptr< MDB_cursor, void ( * )( MDB_cursor* ) > _cursor;
    };

    class Cursor final : public ConstCursor {
        friend class DataBase;

    public:
        void put( const Item& item, const unsigned int flags = 0u ) {
            auto key = createVal( item.key );
            auto value = createVal( item.value );
            if( const auto error = ::mdb_cursor_put( *this, &key, &value, flags ) ) {
                throwIfError( error );
            }
        }

        void erase() {
            if( const auto error = ::mdb_cursor_del( *this, 0 ) ) {
                throwIfError( error );
            }
        }

        Cursor( Cursor&& ) noexcept = default;

    protected:
        using ConstCursor::ConstCursor;
        using ConstCursor::createVal;
    };

public:
    Cursor createCursor( Transaction& txn ) {
        return createCursorImpl( txn );
    }

    ConstCursor createCursor( Transaction& txn ) const {
        return createCursorImpl( txn );
    }

    MDB_stat getStat( Transaction& txn ) const {
        MDB_stat stat;

        if( const auto error = ::mdb_stat( txn, m_dbi, &stat ) ) {
            throwIfError( error );
        }

        return stat;
    }

    void put( Transaction& txn, Item item, const unsigned int flags = 0 ) {
        auto key = Cursor::createVal( item.key );
        auto value = Cursor::createVal( item.value );
        if( const auto error = ::mdb_put( txn, m_dbi, &key, &value, flags ) ) {
            throwIfError( error );
        }
    }

    void clear( Transaction& txn ) {
        if( const auto error = ::mdb_drop( txn, m_dbi, 0 /* to empty the DB */ ) ) {
            throwIfError( error );
        }
    }

    void drop( Transaction& txn ) {
        if( const auto error = ::mdb_drop( txn, m_dbi, 1 /* to delete the DB */ ) ) {
            throwIfError( error );
        }
    }

    void close() {
        ::mdb_dbi_close( m_env, m_dbi );
    }

    DataBase( DataBase&& ) noexcept = default;
    DataBase( const DataBase& ) = delete;

private:
    explicit DataBase( MDB_env* env, MDB_dbi dbi )
    : m_env( env )
    , m_dbi( dbi ) {}

    Cursor createCursorImpl( Transaction& txn ) const {
        MDB_cursor* result = nullptr;
        auto error = MDB_SUCCESS;
        if( ( error = ::mdb_cursor_open( txn, m_dbi, &result ) ) || result == nullptr ) {
            throwIfError( error );
        }
        return Cursor( result );
    }

private:
    MDB_env* m_env = nullptr;
    MDB_dbi m_dbi = MDB_dbi{};
};

class Env final {
public:
    static constexpr std::size_t defaultMapSize = 0U;
    static constexpr std::size_t defaultMaxNamedDB = 0U;

private:
    using Permissions = ::mdb_mode_t;
    static constexpr Permissions defaultPermissions = 0644; /* -rw-r--r-- */

public:
    Env( const std::string& path,
         const unsigned int flags,
         const std::size_t maxNamedDB = defaultMaxNamedDB,
         const std::size_t mapSize = defaultMapSize )
    : m_env(
          []() {
              MDB_env* env = nullptr;
              auto error = MDB_SUCCESS;
              if( ( error = ::mdb_env_create( &env ) ) || env == nullptr ) {
                  throwIfError( error );
              }
              return env;
          }(),
          ::mdb_env_close ) {
        if( maxNamedDB != defaultMaxNamedDB && ::mdb_env_set_maxdbs( *this, maxNamedDB ) ) {
            throw Error( "Could not set up maxdbs:" + std::to_string( maxNamedDB ) );
        }

        if( auto error = ::mdb_env_open( *this, path.c_str(), flags, defaultPermissions ) ) {
            throw Error( "Could not open Env for path: " + path + " error " + std::to_string( error ) );
        }

        if( mapSize != defaultMapSize && ::mdb_env_set_mapsize( *this, mapSize ) ) {
            throw Error( "Could not set map size: " + std::to_string( mapSize ) );
        }
    }

    Transaction createTransaction( const unsigned int flags = 0u, int retryCount = 0 ) const {
        MDB_txn* txn = nullptr;
        auto error = MDB_SUCCESS;
        if( ( error = ::mdb_txn_begin( *this, nullptr, flags, &txn ) ) || txn == nullptr ) {
            if( retryCount == 0 && MDB_MAP_RESIZED == error ) {
                // adopt the size changed by another process
                const int adoptChangedFromOtherProcess = 0L;
                if( ( error = ::mdb_env_set_mapsize( *this, adoptChangedFromOtherProcess ) ) == 0 ) {
                    txn = createTransaction( flags, retryCount + 1 );
                }
                else {
                    throwIfError( error );
                }
            }
            throwIfError( error );
        }
        return Transaction( txn );
    }

    template < typename Key, typename Value >
    DataBase< Key, Value > open( Transaction& txn, const char* name, const unsigned int flags ) {
        return openImpl< Key, Value >( txn, name, flags );
    }

    template < typename Key, typename Value >
    const DataBase< Key, Value > open( Transaction& txn, const char* name, const unsigned int flags ) const {
        return openImpl< Key, Value >( txn, name, flags );
    }

    bool exist( Transaction& txn, const char* name ) const {
        MDB_dbi dbi{};
        int result = ::mdb_dbi_open( txn, name, 0, &dbi );
        return result == 0;
    }

private:
    operator MDB_env*() const noexcept {
        return m_env.get();
    }

    template < typename Key, typename Value >
    DataBase< Key, Value > openImpl( Transaction& txn, const char* name, const unsigned int flags ) const {
        MDB_dbi result{};
        if( const auto error = ::mdb_dbi_open( txn, name, flags, &result ) ) {
            throw Error( "Could not open data base: " + std::string( name ) +
                         ", reason: " + std::string(::mdb_strerror( error ) ) );
        }
        return DataBase< Key, Value >( *this, result );
    }

private:
    std::unique_ptr< MDB_env, void ( * )( MDB_env* ) > m_env;
};

}  // namespace lmdb
