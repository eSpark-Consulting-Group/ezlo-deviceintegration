#include "luad/common/file_descriptor.h"
#include "luad/common/luup_api.h"
#include "luad/old_luup/luup_engine.h"

#include "log_utils/ezlo_log.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <chrono>
#include <cstring>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>

#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

/* The implementation of the luup.io sub-API --
 * http://wiki.micasaverde.com/index.php/Luup_Lua_extensions#Module:_io */

namespace {
extern const luaL_Reg luupIOModule[];
}

using namespace HubPlatform;

class LuupIOAPI final : public LuupAPI {
public:
    LuupIOAPI() {
        registerModule( "io", luupIOModule );
    }

    static int sendData( int fd, const char* buf, std::size_t bufSize );
    static std::size_t getData( int fd, char* buf, std::size_t bufSize );

    static int luupIOOpen( lua_State* luaState );
    static int luupIOWrite( lua_State* luaState );
    static int luupIOIntercept( lua_State* luaState );
    static int luupIORead( lua_State* luaState );
    static int luupIOIsConnected( lua_State* luaState );

private:
    using AsyncOpenRequests = std::map< int /*device id*/, std::future< FileDescriptor > >;

    static void addAsyncOpenRequest( int deviceID, std::future< FileDescriptor > future );
    static std::future< FileDescriptor > findAndRemoveAsyncOpenRequest( int deviceID );

private:
    static std::mutex m_asyncOpenRequestsMtx;
    static AsyncOpenRequests m_asyncOpenRequests;

    static const int m_segmentSize = 4096; /* MiOS reads up to 4K bytes by one read call */
};

class IOError : public std::runtime_error {
public:
    IOError( const std::string& errDesription )
    : runtime_error( "io." + errDesription ) {}
};

std::mutex LuupIOAPI::m_asyncOpenRequestsMtx;
LuupIOAPI::AsyncOpenRequests LuupIOAPI::m_asyncOpenRequests;

namespace {

const luaL_Reg luupIOModule[] = { { "open", LuupIOAPI::luupIOOpen },
                                  { "write", LuupIOAPI::luupIOWrite },
                                  { "intercept", LuupIOAPI::luupIOIntercept },
                                  { "read", LuupIOAPI::luupIORead },
                                  { "is_connected", LuupIOAPI::luupIOIsConnected },
                                  { NULL, NULL } };
}

static LuupIOAPI luupIOAPI;

int LuupIOAPI::sendData( int fd, const char* buf, std::size_t bufSize ) {
    std::size_t currentOffset = 0;
    int ret;

    while( true ) {
        /* try to write up to dataLen bytes to the fd */
        ret = send( fd, buf + currentOffset, bufSize, MSG_NOSIGNAL );
        if( ret < 0 && errno == EINTR )
            continue;

        /* keep it up till all data is written */
        if( ret < 0 && ( errno == EWOULDBLOCK || errno == EAGAIN ) )
            continue;

        if( ret < 0 )
            return ret;

        bufSize -= ret;
        currentOffset += ret;

        /* no more data to write */
        if( bufSize == 0 )
            break;
    }

    return 0;
}

std::size_t LuupIOAPI::getData( int fd, char* buf, std::size_t bufSize ) {
    std::size_t available_space, current_offset;
    int ret;

    available_space = bufSize;
    current_offset = 0;

    while( true ) {
        /* try to read up to buff.size() bytes from the input stream */
        ret = recv( fd, buf + current_offset, available_space, MSG_DONTWAIT );
        if( ret < 0 && errno == EINTR )
            continue;

        /* no more data to read */
        if( ret < 0 && errno == EWOULDBLOCK )
            break;

        if( ret < 0 )
            throw std::system_error{ errno, std::system_category(), "getData: read" };

        available_space -= ret;
        current_offset += ret;

        if( available_space == 0 )
            break;
    }

    return bufSize - available_space;
}

std::future< FileDescriptor > LuupIOAPI::findAndRemoveAsyncOpenRequest( int deviceID ) {
    std::lock_guard< std::mutex > lk( m_asyncOpenRequestsMtx );
    std::future< FileDescriptor > future;

    auto it = m_asyncOpenRequests.find( deviceID );
    if( it != m_asyncOpenRequests.end() ) {
        future = std::move( ( *it ).second );
        m_asyncOpenRequests.erase( it );
    }

    return future;
}

void LuupIOAPI::addAsyncOpenRequest( int deviceID, std::future< FileDescriptor > future ) {
    std::lock_guard< std::mutex > lk( m_asyncOpenRequestsMtx );

    auto res = m_asyncOpenRequests.emplace( deviceID, std::move( future ) );
    if( !res.second ) {
        throw IOError{ " : an open request for this device exists already" };
    }
}

/* opens a remote connection to host(ip):port
 * arguments: device (string or number), ip (string), port (as number or string);
 * return: nothing */
int LuupIOAPI::luupIOOpen( lua_State* luaState ) {
    if( lua_gettop( luaState ) != 3 )
        throw IOError{ "open: an incorrect number of arguments" };

    /* TODO: now support only a device id */
    auto deviceID = lua_type( luaState, 1 ) == LUA_TNUMBER ? lua_tonumber( luaState, 1 ) :
                                                             throw IOError{ "open: an incorrect 'device' argument" };

    auto host = lua_type( luaState, 2 ) == LUA_TSTRING ? lua_tostring( luaState, 2 ) :
                                                         throw IOError{ "open: an incorrect 'ip' argument" };

    /* a port as a number can be converted to the port as a string freely */
    auto port = lua_isstring( luaState, 3 ) ? lua_tostring( luaState, 3 ) :
                                              throw IOError{ "open: an incorrect 'port' argument" };

    /* TODO: the API says nothing about such case, so just use an already open connection */
    if( Device::makeDevice( deviceID ).isConnectionEstablished() ) {
        Log::warn( "io.open: a device: " + std::to_string( deviceID ) + " has an open connection already" );
        return 0;
    }

    auto op = []( const std::string& host, const std::string& port ) {
        addrinfo searchHints, *searchResults;

        std::memset( &searchHints, 0, sizeof( searchHints ) );
        searchHints.ai_family = AF_UNSPEC;
        searchHints.ai_socktype =
            SOCK_STREAM; /* the API says nothing about type of connection, but MiOS uses a TCP one */
        searchHints.ai_protocol = 0;
        searchHints.ai_flags = AI_ADDRCONFIG;

        /* Note: the API requires "stores the handle to the socket in 'device'", but it seems we can
         * defer this 'storing' till we have some access to the socket, thus behavior allow us to
         * use a DNS lookup, thus allow to use host names, not only ip addresses (remember about IPv6) */
        if( auto res = getaddrinfo( host.c_str(), port.c_str(), &searchHints, &searchResults ) ) {
            Log::error( std::string{ "io.open (async): fail to get socket address(es) to connect to: " } +
                        gai_strerror( res ) );
            return FileDescriptor{};
        }

        std::unique_ptr< addrinfo, decltype( &freeaddrinfo ) > results( searchResults, freeaddrinfo );

        for( auto res = results.get(); res; res = res->ai_next ) {
            FileDescriptor sock( socket( res->ai_family, res->ai_socktype | SOCK_CLOEXEC, 0 ) );
            if( sock < 0 ) {
                /* TODO: Log isn't thread-safe... */
                Log::error( "io.open (async): fail to create a socket: " + getLastErrnoDescription() );
                return FileDescriptor{};
            }

            int keepAlive = 1;
            if( setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof( keepAlive ) ) ) {
                Log::error( "io.open (async): fail to set a socket option: " + getLastErrnoDescription() );
                return FileDescriptor{};
            }

            if( connect( sock, res->ai_addr, res->ai_addrlen ) ) {
                Log::warn( "io.open (async): fail to connect to specified socket address: " +
                           getLastErrnoDescription() );
                continue;
            }
            else
                return sock;
        }

        Log::error( std::string{ "io.open (async): fail to connect to the specified host:port" } + " (" + host + ':' +
                    port + ')' );
        return FileDescriptor{};
    };

    try {
        /* the API requires an operation being performed asynchronously */
        /* TODO: agree on this async operation with an addon policy */
        auto future = std::async( std::launch::async, op, std::string{ host }, std::string{ port } );
        addAsyncOpenRequest( deviceID, std::move( future ) );
    }
    catch( const std::exception& ex ) {
        Log::warn( std::string{ "io.open: fail to run an async open request: " } + ex.what() );
    }
    catch( ... ) {
        Log::warn( "io.open: fail to run an async open request" );
    } /* it's not a disaster from the API point's of view */

    return 0;
}

/* arguments: data^ (string, which may contain raw binary data), device (string or number);
 * return: result (boolean) */
int LuupIOAPI::luupIOWrite( lua_State* luaState ) {
    if( lua_gettop( luaState ) < 1 || lua_gettop( luaState ) > 2 )
        throw IOError{ "write: an incorrect number of arguments" };

    std::size_t dataLen;
    auto data = lua_tolstring( luaState, 1, &dataLen );
    if( !data )
        throw IOError{ "write: an incorrect 'data' argument" };

    /* TODO: now support only a device id */
    /* TODO: get a self id if no id provided */
    int deviceID = lua_type( luaState, 2 ) == LUA_TNUMBER ? lua_tointeger( luaState, 2 ) : 0;

    auto dev = Device::makeDevice( deviceID );
    FileDescriptor sock;

    /* TODO: the API says nothing about such case, so just use previous one */
    if( dev.isConnectionEstablished() ) {
        /* TODO: a future obtained from std::async makes us wait till it gets triggered, even if
         *       we don't want it... */
        findAndRemoveAsyncOpenRequest( deviceID );
        sock = FileDescriptor{ dev.getConnectionSocket() };
    }
    else {
        auto future = findAndRemoveAsyncOpenRequest( deviceID );

        if( !future.valid() ) {
            Log::error( "io.write: a future we have to wait on isn't valid" );

            lua_pushboolean( luaState, 0 );
            return 1;
        }

        /* the API allows to wait up to 5 seconds */
        auto status = future.wait_for( std::chrono::seconds( 5 ) );

        if( status == std::future_status::timeout ) {
            Log::error( "io.write: fail to establish the connection during a timeout" );

            lua_pushboolean( luaState, 0 );
            return 1;
        }
        else if( status == std::future_status::deferred )
            throw IOError{ "write: an async open request hasn't been started" };

        try {
            sock = future.get();
            if( !sock )
                throw 42; /* just to generalize the code */

            dev.setConnectionSocket( sock );
        }
        catch( const std::exception& ex ) {
            Log::warn( std::string{ "io.write: an async connection establishment has failed: " } + ex.what() );

            lua_pushboolean( luaState, 0 );
            return 1;
        }
        catch( ... ) {
            Log::error( "io.write: an async connection establishment has failed" );

            lua_pushboolean( luaState, 0 );
            return 1;
        }
    }

    /* TODO: data has to be processed before we're able to send them, as the API specifies;
     *       keep in mind that the 'data' variable is a pointer to an object located within Lua state */
    /* TODO: the API says nothing about should this connection be non-blocking or not? */
    if( sendData( sock, data, dataLen ) < 0 ) {
        Log::error( "io.write: fail to send data: " + getLastErrnoDescription() );

        lua_pushboolean( luaState, 0 );
        return 1;
    }

    lua_pushboolean( luaState, 1 );
    return 1;
}

/* arguments: device (string or number);
 * return: nothing */
int LuupIOAPI::luupIOIntercept( lua_State* luaState ) {
    if( lua_gettop( luaState ) != 1 )
        throw IOError{ "intercept: an incorrect number of arguments" };

    /* TODO: now support only a device id */
    /* TODO: get a self id if no id provided */
    int deviceID = lua_type( luaState, 1 ) == LUA_TNUMBER ? lua_tointeger( luaState, 1 ) : 0;

    auto dev = Device::makeDevice( deviceID );
    dev.setInterceptionMode( Device::InterceptionMode::Pass );

    return 0;
}

/* arguments: timeout (number), device (string or number);
 * return: data (string) */
int LuupIOAPI::luupIORead( lua_State* luaState ) {
    if( lua_gettop( luaState ) != 2 )
        throw IOError{ "read: an incorrect number of arguments" };

    /* timeout is specified in seconds */
    auto timeout = ( lua_type( luaState, 1 ) == LUA_TNUMBER ? lua_tonumber( luaState, 2 ) : 5 ) >= 0 ?: 5;

    /* TODO: now support only a device id */
    /* TODO: get a self id if no id provided */
    int deviceID = lua_type( luaState, 2 ) == LUA_TNUMBER ? lua_tointeger( luaState, 2 ) : 0;

    auto dev = Device::makeDevice( deviceID );

    pollfd eventsFd[ 1 ];

    std::memset( eventsFd, 0, sizeof( eventsFd ) );
    eventsFd[ 0 ].events = POLLIN;
    eventsFd[ 0 ].fd = dev.getConnectionSocket();

    while( true ) {
        int ret;

        ret = poll( eventsFd, sizeof( eventsFd ) / sizeof( pollfd ), timeout * 1000 );
        if( ret < 0 && errno == EINTR )
            continue;

        if( ret < 0 )
            throw std::system_error{ errno, std::system_category(), "poll" };

        if( ret == 0 ) {
            Log::error( "io.read: no data to read during a timeout" );
            return 0;
        }

        if( eventsFd[ 0 ].revents == POLLIN ) {
            char buff[ m_segmentSize ];

            /* TODO: the API says nothing about should this operation be non-blocking or not? */
            auto actualSize = getData( eventsFd[ 0 ].fd, buff, sizeof( buff ) );

            lua_pushlstring( luaState, buff, actualSize );
            return 1;
        }

        throw IOError{ "read: something bad happened" };
    }
}

/* arguments: device (string or number);
 * return: connected (boolean) */
int LuupIOAPI::luupIOIsConnected( lua_State* luaState ) {
    if( lua_gettop( luaState ) != 1 )
        throw IOError{ "is_connected: an incorrect number of arguments" };

    /* TODO: now support only a device id */
    /* TODO: get a self id if no id provided */
    int deviceID = lua_type( luaState, 1 ) == LUA_TNUMBER ? lua_tointeger( luaState, 1 ) : 0;

    auto dev = Device::makeDevice( deviceID );
    if( dev.isConnectionEstablished() ) {
        lua_pushboolean( luaState, 1 );
        return 1;
    }

    lua_pushboolean( luaState, 0 );
    return 1;
}
