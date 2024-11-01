#include "luad/modules/network/network_manager.h"
#include "luad/modules/network/events_emitter.h"
#include "luad/addon/lua_addon.h"

#include "binding_interfaces/lua/lua_binding_interface.h"
#include "common_utils/cpp14.h"

#include "json_utils/json_utils.h"

/* see the documentation on http://log.ezlo.lan/new/scripting/Network */

#define CHECK_LUA_INPUT_PARAMETERS( callFunction, number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "network." } + ( callFunction ) ), number )

#define CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( callFunction, number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "network." } + ( callFunction ) ), number )

#define GET_LUA_INPUT_PARAMETERS( type, idx, callFunction ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "network." } + ( callFunction ) ) )

namespace HubPlatform {
namespace {

using namespace LuaNetwork;

class LuaEventsEmitter final : public EventsEmitter {
public:
    explicit LuaEventsEmitter( LuaAddon::LuaAddon& luaAddon )
    : m_luaAddon( luaAddon ) {}

    void emitConnectionEstablishedEvent( const std::string& eventHandler, int connectionHandle ) override {
        const auto event = prepareEvent( "CONNECTED", connectionHandle );
        emitEvent( eventHandler, event );
    }

    void emitFailedToConnectEvent( const std::string& eventHandler, int connectionHandle ) override {
        const auto event = prepareEvent( "FAIL_TO_CONNECT", connectionHandle );
        emitEvent( eventHandler, event );
    }

    void emitIncomingDataEvent( const std::string& eventHandler, int connectionHandle ) override {
        const auto event = prepareEvent( "IN", connectionHandle );
        emitEvent( eventHandler, event );
    }

    void emitConnectionClosedEvent( const std::string& eventHandler, int connectionHandle ) override {
        const auto event = prepareEvent( "CONNECTION_CLOSED", connectionHandle );
        emitEvent( eventHandler, event );
    }

    void emitScanningResult( const std::string& eventHandler,
                             int scanId,
                             const std::string& status,
                             ScanningOptionResult&& optionResult ) override {
        JsonObject result = JSON( "scan_id" << scanId << "status" << status );
        if( !optionResult ) {
            emitEvent( eventHandler, result );
            return;
        }
        result[ "ip_address" ] = optionResult->ipAddress;
        result[ "mac" ] = optionResult->macAddress;
        result[ "host_name" ] = optionResult->hostName;
        result[ "ports" ] = optionResult->ports;
        result[ "tries" ] = optionResult->tries;
        result[ "errors" ] = optionResult->errors;
        emitEvent( eventHandler, result );
    }

    void emitDhcpEvent( const std::string& eventHandler,
                        const std::string& eventType,
                        const Json& eventData ) override {
        m_luaAddon.localEmit( eventHandler, eventType, eventData );
    }

    void emitReceivedFromEvent( std::string eventHandler,
                                int connectionHandle,
                                std::string userData,
                                std::string data,
                                std::string address,
                                std::string port ) override {
        const auto event = JSON( "handle" << connectionHandle << "user_data" << userData << "datagram" << data
                                          << "address" << address << "port" << port );
        m_luaAddon.localEmit( eventHandler, "requestfrom", event );
    }

    void emitReceivedEvent( std::string eventHandler,
                            int connectionHandle,
                            std::string userData,
                            std::string data ) override {
        const auto event = JSON( "handle" << connectionHandle << "user_data" << userData << "data" << data );
        m_luaAddon.localEmit( eventHandler, "request", event );
    }

private:
    Json prepareEvent( const std::string& eventName, int connectionHandle ) const {
        return JSON( "event_type"
                     << "io_activity"
                     << "event" << JSON( "handle" << connectionHandle << "io_activity_type" << eventName ) );
    }

    void emitEvent( const std::string& eventHandler, const Json& event ) {
        /* the documentation states that it's not an error if there's no events handler script subscribed
         * by either 'network.subscribe' or 'network.set_handler`, so we just silence return */
        if( eventHandler.empty() )
            return;

        m_luaAddon.localEmit( eventHandler, "network", event );
    }

private:
    LuaAddon::LuaAddon& m_luaAddon;
};

NetworkManager& getNetworkManager( lua_State* luaStates ) {
    auto& service = dynamic_cast< LuaAddon::LuaAddon& >( getHostService( luaStates ) );

    static auto networkManager = [&service]() {
        auto networkManager =
            std::make_shared< NetworkManager >( service, std::make_unique< LuaEventsEmitter >( service ) );

        std::weak_ptr< NetworkManager > networkManagerWeak( networkManager );
        auto resetPluginInfoCallback = [networkManagerWeak]( const std::string& pluginId ) {
            auto networkManagerLock = networkManagerWeak.lock();
            if( networkManagerLock ) {
                networkManagerLock->reset( pluginId );
            }
        };

        service.subscribeOnPluginRestarted( resetPluginInfoCallback );
        service.subscribeOnPluginTeardown( resetPluginInfoCallback );
        service.subscribeOnPluginUninstall( resetPluginInfoCallback );

        return networkManager;
    }();

    return *networkManager;
}

ConnectionType getConnectionType( const std::string& connectionType ) {
    if( connectionType == "TCP" )
        return ConnectionType::TCP;
    if( connectionType == "UDP" )
        return ConnectionType::UDP;

    return ConnectionType::NONE;
}

RemoteAddress getRemoteAddress( lua_State* luaState, int idx ) {
    auto ip = lookupLuaTable< NativeString >( luaState, "ip", idx, true );
    auto port = lookupLuaTable< NativeString >( luaState, "port", idx, true );
    auto connectionType = lookupLuaTable< NativeString >( luaState, "connection_type", idx, true );

    auto type = getConnectionType( connectionType.value() );
    if( type == ConnectionType::NONE )
        luaL_error( luaState, "getRemoteAddress: incorrect type of a connection" );

    return RemoteAddress{ ip.value(), port.value(), type };
}

/* arguments: name of an events handler script (string).
 * return: nothing. */
int subscribe( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    auto luaAPIRestrictions = getLuaModuleAPIRestrictions( luaState );
    if( luaAPIRestrictions == HubAPIRestrictions::NO_ASYNC_RESTRICTION ) {
        Log::warn( cc::warn(
            "Attempt to use eventsSubscriber functionality without proper permissions, current permission is " +
            cc::note( getHubAPIRestrictionsString( luaAPIRestrictions ) ) ) );
        return luaL_error( luaState, "fail to subscribe" );
    }

    const auto eventsHandler = GET_LUA_INPUT_PARAMETERS( NativeString, 1, __func__ );

    try {
        getNetworkManager( luaState ).setEventsHandler( getPluginId( luaState ), eventsHandler );
    }
    catch( const std::exception& exception ) {
        return luaL_error(
            luaState, "fail to set a global event handler(%s) : %s", eventsHandler.c_str(), exception.what() );
    }

    return 0;
}

/* throws an exception if something local went wrong
 * arguments: remote address (table), options (int) [optional].
 * return: connection handle (int). */
int connect( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    const auto remoteAddress = getRemoteAddress( luaState, 1 );
    const auto connectOptions = LuaToNative< NativeInteger >{}( luaState, 2 );

    try {
        auto& netManager = getNetworkManager( luaState );

        auto handle = netManager.connect( getPluginId( luaState ), remoteAddress, connectOptions.valueOr( 0 ) );
        lua_pushinteger( luaState, handle );

        return 1;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState,
                           "fail to make a connection (%s) to a remote %s:%s : %s",
                           std::get< 2 >( remoteAddress ) == ConnectionType::TCP ? "TCP" : "UDP",
                           std::get< 0 >( remoteAddress ).c_str(),
                           std::get< 1 >( remoteAddress ).c_str(),
                           exception.what() );
    }
}

/* arguments: hndl (int), binary data to send (string).
 * return: amount of sent data (int). */
int send( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 2 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto data = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );

    try {
        auto& netManager = getNetworkManager( luaState );

        auto sentAmount = netManager.send( getPluginId( luaState ), connectionHandle, data );
        lua_pushinteger( luaState, sentAmount );

        return 1;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error(
            luaState, "fail to send data to a connection with handle (%d) : %s", connectionHandle, exception.what() );
    }
}

/* arguments: hndl (int), binary data to send (string), address(string), port(string)
 * return: amount of sent data (int). */
int sendto( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 4 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto data = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );
    const auto address = GET_LUA_INPUT_PARAMETERS( NativeString, 3, __func__ );
    const auto port = GET_LUA_INPUT_PARAMETERS( NativeString, 4, __func__ );

    try {
        auto& netManager = getNetworkManager( luaState );

        auto sentAmount = netManager.sendto( getPluginId( luaState ), connectionHandle, data, address, port );
        lua_pushinteger( luaState, sentAmount );

        return 1;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error(
            luaState, "fail to send data to a connection with handle (%d) : %s", connectionHandle, exception.what() );
    }
}

/* arguments: hndl (int), count, up to, to receive (int) [optional]
 * return: binary data (string).
 * Note: this call fetches data from an internal buffer, to inspect data without fetching them use
 * 'network.inspect_data' */
int receive( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto count = LuaToNative< NativeInteger >{}( luaState, 2 ).valueOr( IBuffer::MaxCapacity );

    try {
        auto& netManager = getNetworkManager( luaState );

        auto data = netManager.receive( getPluginId( luaState ), connectionHandle, count );
        lua_pushlstring( luaState, data.c_str(), data.size() );

        return 1;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState,
                           "fail to receive data from a connection with handle (%d) : %s",
                           connectionHandle,
                           exception.what() );
    }
}

/* arguments: hndl (int), size (int)
 * return: nothing. */
int receiveFrom( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto count = LuaToNative< NativeInteger >{}( luaState, 2 ).valueOr( IBuffer::MaxCapacity );

    try {
        auto& netManager = getNetworkManager( luaState );

        std::string data, address, port;
        std::tie( data, address, port ) = netManager.receiveFrom( getPluginId( luaState ), connectionHandle, count );

        lua_pushlstring( luaState, data.c_str(), data.size() );
        lua_pushstring( luaState, address.c_str() );
        lua_pushstring( luaState, port.c_str() );

        return 3;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState,
                           "fail to receive data from a connection with handle (%d) : %s",
                           connectionHandle,
                           exception.what() );
    }
}

/* arguments: hndl (int), callback(string), user_data(string) [optional], size (int) [optional]
 * return: nothing. */
int request( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    auto callback = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );
    auto userData = LuaToNative< NativeString >{}( luaState, 3 ).valueOr( std::string() );
    auto count = LuaToNative< NativeInteger >{}( luaState, 4 ).valueOr( IBuffer::MaxCapacity );

    try {
        auto& netManager = getNetworkManager( luaState );

        netManager.request(
            getPluginId( luaState ), connectionHandle, std::move( callback ), std::move( userData ), count );

        return 0;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState,
                           "fail to receive data from a connection with handle (%d) : %s",
                           connectionHandle,
                           exception.what() );
    }
}

/* arguments: hndl (int), callback(string), user_data(string) [optional], size (int) [optional]
 * return: nothing. */
int requestFrom( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    auto callback = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );
    auto userData = LuaToNative< NativeString >{}( luaState, 3 ).valueOr( std::string() );
    auto count = LuaToNative< NativeInteger >{}( luaState, 4 ).valueOr( IBuffer::MaxCapacity );

    try {
        auto& netManager = getNetworkManager( luaState );

        netManager.requestFrom(
            getPluginId( luaState ), connectionHandle, std::move( callback ), std::move( userData ), count );

        return 0;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState,
                           "fail to receive data from a connection with handle (%d) : %s",
                           connectionHandle,
                           exception.what() );
    }
}

/* arguments: hndl (int), count, up to, to receive (int) [optional].
 * return: binary data (string). */
int inspectData( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto count = LuaToNative< NativeInteger >{}( luaState, 2 );

    try {
        auto& netManager = getNetworkManager( luaState );

        auto data = netManager.inspectData(
            getPluginId( luaState ), connectionHandle, count ? count.value() : IBuffer::MaxCapacity );
        lua_pushlstring( luaState, data.c_str(), data.size() );

        return 1;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState,
                           "fail to inspect data for a connection with handle (%d) : %s",
                           connectionHandle,
                           exception.what() );
    }
}

/* arguments: hndl (int).
 * return: size of received data, in bytes (int). */
int inspectDataSize( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );

    try {
        auto& netManager = getNetworkManager( luaState );

        auto data = netManager.inspectDataSize( getPluginId( luaState ), connectionHandle );
        lua_pushinteger( luaState, data );

        return 1;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState,
                           "fail to inspect data size for a connection with handle (%d) : %s",
                           connectionHandle,
                           exception.what() );
    }
}

/* arguments: hndl (int), name of an events handler script (string).
 * return: nothing. */
int setHandler( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 2 );

    auto luaAPIRestrictions = getLuaModuleAPIRestrictions( luaState );
    if( luaAPIRestrictions == HubAPIRestrictions::NO_ASYNC_RESTRICTION ) {
        Log::warn( cc::warn(
            "Attempt to use eventsSubscriber functionality without proper permissions, current permission is " +
            cc::note( getHubAPIRestrictionsString( luaAPIRestrictions ) ) ) );
        return luaL_error( luaState, "fail to setHandler/subscribe" );
    }

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto eventsHandler = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );

    try {
        getNetworkManager( luaState ).setEventsHandler( getPluginId( luaState ), eventsHandler, connectionHandle );
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState,
                           "fail to set an event handler(%s) for a handle(%d) : %s",
                           eventsHandler.c_str(),
                           connectionHandle,
                           exception.what() );
    }

    return 0;
}

/* arguments: hndl (int).
 * return: nothing. */
int close( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );

    try {
        getNetworkManager( luaState ).close( getPluginId( luaState ), connectionHandle );
    }
    catch( const std::exception& exception ) {
    }

    return 0;
}

/* arguments: script (string), ip addresses (string), ip version (string).
 * return: scan id ( int ). */
int startScanNetwork( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 3 );

    const auto resultHandler = GET_LUA_INPUT_PARAMETERS( NativeString, 1, __func__ );
    const auto addresses = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );
    const auto ipVersion = GET_LUA_INPUT_PARAMETERS( NativeString, 3, __func__ );
    const auto portsInt = LuaToNative< NativeInt32Vector >{}( luaState, 4 );
    const auto tries = LuaToNative< NativeInteger >{}( luaState, 5 ).valueOr( 1 );

    NetworkScanClient::AddressVersion addressVersion;
    if( ipVersion == "IPv4" ) {
        addressVersion = NetworkScanClient::AddressVersion::IPv4;
    }
    else if( ipVersion == "IPv6" ) {
        addressVersion = NetworkScanClient::AddressVersion::IPv6;
    }
    else {
        return luaL_error( luaState, "invalid IP version : %s", ipVersion.c_str() );
    }

    INetworkScan::Addresses networkScanAddresses;
    const char listSeparator = ',';
    const char rangeSeparator = '-';
    auto isCommaFound = addresses.find( listSeparator ) != std::string::npos;
    auto isHyphenFound = addresses.find( rangeSeparator ) != std::string::npos;
    if( !isHyphenFound ) {
        AddressList::Addresses ipAddresses;
        std::string::size_type pos = 0;
        for( int i = 0; true; ++i, ++pos ) {
            auto oldPos = pos;
            pos = addresses.find( listSeparator, pos );
            if( auto ipAddress = toIpAddress( addressVersion, addresses.substr( oldPos, pos ), 0 ) ) {
                ipAddresses.push_back( std::move( ipAddress ) );
            }
            else {
                return luaL_error( luaState, "invalid address format : %s", addresses.c_str() );
            }
            if( pos == std::string::npos ) {
                break;
            }
        }
        networkScanAddresses = std::make_unique< AddressList >( std::move( ipAddresses ) );
    }
    else if( !isCommaFound && isHyphenFound ) {
        AddressRange::AddressPtr min;
        AddressRange::AddressPtr max;
        auto pos = addresses.find( rangeSeparator, 0 );
        if( !( min = toIpAddress( addressVersion, addresses.substr( 0, pos ), 0 ) ) ) {
            return luaL_error( luaState, "invalid address format : %s", addresses.c_str() );
        }
        pos++;
        if( !( max = toIpAddress( addressVersion, addresses.substr( pos, std::string::npos ), 0 ) ) ) {
            return luaL_error( luaState, "invalid address format : %s", addresses.c_str() );
        }
        networkScanAddresses = std::make_unique< AddressRange >( std::move( min ), std::move( max ) );
    }
    else {
        return luaL_error( luaState, "invalid address format : %s", addresses.c_str() );
    }

    try {
        auto scanId = getNetworkManager( luaState )
                          .startScanning( getPluginId( luaState ),
                                          addressVersion,
                                          std::move( networkScanAddresses ),
                                          std::move( portsInt ? portsInt.value() : NativeInt32Vector() ),
                                          resultHandler,
                                          tries );
        lua_pushinteger( luaState, scanId );
        return 1;
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState, "fail to start scanning : %s", exception.what() );
    }

    return 0;
}

/* arguments: scan id (int).
 * return: nothing. */
int stopScanNetwork( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto scanId = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );

    try {
        getNetworkManager( luaState ).stopScanning( getPluginId( luaState ), scanId );
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState, "fail to stop scanning : %s", exception.what() );
    }

    return 0;
}

/** Lua Arguments:
 *      path: string (required) - path to a script to receive DHCP events
 *      operations: string[] (optional) - array of operations to listen for.
 *  @return subscription Id.
 */
int startListeningDhcp( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    const auto clientScript = GET_LUA_INPUT_PARAMETERS( NativeString, 1, __func__ );
    const auto operationsArr = LuaToNative< NativeStringVector >{}( luaState, 2 );

    Json arr( operationsArr ? operationsArr.value() : NativeStringVector() );
    Log::trace( std::string( "# DHCP subscription initiated for script: '" ) + clientScript +
                "', operations: " + arr.dump() );

    try {
        NetworkManager& network = getNetworkManager( luaState );
        auto subscriptId = network.startListeningDhcp(
            getPluginId( luaState ), clientScript, operationsArr ? operationsArr.value() : NativeStringVector() );

        LOGGER_TRACE_RAW( cc::trace( "# DHCP subscription created for script: '" ) + clientScript +
                          "', id: " + subscriptId );

        lua_pushlstring( luaState, subscriptId.c_str(), subscriptId.size() );
        return 1;
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState, "fail to start DHCP listening: %s", exception.what() );
    }
}

/* arguments: subscription id (int).
 * return: nothing. */
int stopListeningDhcp( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 1 );

    const auto subscriptionId = GET_LUA_INPUT_PARAMETERS( NativeString, 1, __func__ );
    LOGGER_TRACE_RAW( cc::trace( "# DHCP unsubscribe, sessionId : '" ) + subscriptionId + "'" );

    std::string okString{ "Ok" };
    try {
        getNetworkManager( luaState ).stopListeningDhcp( getPluginId( luaState ), subscriptionId );
        LOGGER_TRACE_RAW( "DHCP stop: storing value: " + okString );
        lua_pushlstring( luaState, okString.data(), okString.size() );
        return 1;
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState,
                           "fail to stop listening DHCP events for subscription id '%s' : %s",
                           subscriptionId.c_str(),
                           exception.what() );
    }

    return 0;
}

/* arguments: no.
 * return: nothing. */
int udp( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( __func__, 0 );

    try {
        auto& netManager = getNetworkManager( luaState );

        auto handle = netManager.createUdp( getPluginId( luaState ) );
        lua_pushinteger( luaState, handle );

        return 1;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState, "fail to create UDP socket: %s", exception.what() );
    }
}

/* arguments:hndl (int), address (string), port(string).
 * return: nothing. */
int setPeerName( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 2 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto address = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );
    const auto optionalPort = LuaToNative< NativeString >{}( luaState, 3 );
    const auto port = optionalPort.isValid() ? optionalPort.value() : "0";

    try {
        getNetworkManager( luaState ).setPeerName( getPluginId( luaState ), connectionHandle, address, port );
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState, "fail to set peer name: %s", exception.what() );
    }

    return 0;
}

/* arguments: hndl (int), address (string), port(int).
 * return: nothing. */
int setSockName( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 2 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto address = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );
    const auto optionalPort = LuaToNative< NativeString >{}( luaState, 3 );
    const auto port = optionalPort.isValid() ? optionalPort.value() : "0";

    try {
        getNetworkManager( luaState ).setSockName( getPluginId( luaState ), connectionHandle, address, port );
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState, "fail to set option: %s", exception.what() );
    }

    return 0;
}

/* arguments: hndl (int), option (string), value(bool).
 * return: nothing. */
int setOption( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 2 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto option = GET_LUA_INPUT_PARAMETERS( NativeString, 2, __func__ );
    auto optionValue = pullJson( luaState, 3 );

    try {
        getNetworkManager( luaState ).setOption( getPluginId( luaState ), connectionHandle, option, optionValue );
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState, "fail to set sock name: %s", exception.what() );
    }

    return 0;
}

/* arguments: hndl (int)
 * return: address (string), port(string) */
int getsockname( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 1 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto count = LuaToNative< NativeInteger >{}( luaState, 2 ).valueOr( IBuffer::MaxCapacity );

    try {
        auto& netManager = getNetworkManager( luaState );

        std::string address, port;
        std::tie( address, port ) = netManager.getsockname( getPluginId( luaState ), connectionHandle );

        lua_pushstring( luaState, address.c_str() );
        lua_pushstring( luaState, port.c_str() );

        return 2;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState,
                           "fail to get local address information associated to handle (%d) : %s",
                           connectionHandle,
                           exception.what() );
    }
}

/* arguments: none
 * return: array of handles (array of ints) */
int getSockets( lua_State* luaState ) {
    try {
        auto sockets = JSON_EMPTY_ARR;

        auto& netManager = getNetworkManager( luaState );
        netManager.processOpenedSockets( getPluginId( luaState ),
                                         [&sockets]( int handleId ) { sockets.push_back( handleId ); } );

        pushJsonAsLuaObj( luaState, sockets );

        return 1;
    }
    catch( const std::exception& exception ) {
        /* convert a c++ exception to a lua one */
        return luaL_error( luaState, "fail to get information about opened sockets: %s", exception.what() );
    }
}

int setTimeout( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( __func__, 2 );

    const auto connectionHandle = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1, __func__ );
    const auto timeout = GET_LUA_INPUT_PARAMETERS( NativeInteger, 2, __func__ );
    auto timeoutMs = std::chrono::milliseconds( timeout );
    try {
        getNetworkManager( luaState ).setTimeout( getPluginId( luaState ), connectionHandle, timeoutMs );
    }
    catch( const std::exception& exception ) {
        return luaL_error( luaState, "failed to set timeout: %s", exception.what() );
    }

    return 0;
}

}  // namespace

const char moduleName[] = "network";
const luaL_Reg module[] = { { "subscribe", subscribe },
                            { "connect", connect },
                            { "send", send },
                            { "receive", receive },
                            { "receivefrom", receiveFrom },
                            { "request", request },
                            { "requestfrom", requestFrom },
                            { "inspect_data", inspectData },
                            { "inspect_data_size", inspectDataSize },
                            { "set_handler", setHandler },
                            { "close", close },
                            { "start_scan_network", startScanNetwork },
                            { "stop_scan_network", stopScanNetwork },
                            { "start_listening_dhcp", startListeningDhcp },
                            { "stop_listening_dhcp", stopListeningDhcp },
                            { "udp", udp },
                            { "setpeername", setPeerName },
                            { "setsockname", setSockName },
                            { "setoption", setOption },
                            { "sendto", sendto },
                            { "getsockname", getsockname },
                            { "get_sockets", getSockets },
                            { "set_timeout", setTimeout },
                            { nullptr, nullptr } };

}  // namespace HubPlatform

HUB_BASE_MODULE_ENTRY_DEFINITION( network, HubPlatform::module, HubPlatform::moduleName );
