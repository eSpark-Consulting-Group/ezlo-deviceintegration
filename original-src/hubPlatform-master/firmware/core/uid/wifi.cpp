#include <algorithm>
#include <thread>

#include "wifi.h"
#include "base_utils/utils.h"
#include "log_utils/ezlo_log.h"
#include "jsonrpc/jsonrpc.h"

#include <tiny/tiny_consolecolors.h>

bool Wifi::executeScript( const std::string& name, std::vector< std::string >& result, JsonObject& answer ) {
    char line[ 500 ];
    FILE* in;
    std::string request = "/opt/firmware/scripts/" + name;
    Log::debug( cc::debug( "POPEN: " ) + cc::warn( request ) );
    if( !( in = popen( request.c_str(), "r" ) ) ) {
        Log::error( cc::error( "Script open error: " ) + cc::warn( request ) );
        setError( answer, Errors::SCRIPT_OPEN_ERROR );
        return false;
    }
    result.clear();
    Log::debug( "Script executed: " + request );
    while( fgets( line, sizeof( line ), in ) != NULL ) {
        std::string str( line );
        str.erase( std::remove_if( str.begin(), str.end(), (int ( * )( int )) std::iscntrl ), str.end() );
        Log::debug( "Script lines: " + str );
        result.push_back( str );
    }
    if( pclose( in ) ) {
        Log::error( cc::error( "Script error: " ) + cc::warn( request ) );
        setError( answer, Errors::SCRIPT_ERROR );
        return false;
    }
    return true;
}

bool Wifi::getList( JsonObject& answer ) {
    JsonArray list;
    std::vector< std::string > lines;
    if( !executeScript( "wf-cmd.sh list", lines, answer ) )
        return false;
    for( auto line : lines ) {
        JsonObject jsonLine;
        parseJson( line, jsonLine );
        list.push_back( jsonLine );
    }
    JsonObject result;
    result[ "wifiList" ] = list;
    result[ "count" ] = lines.size();
    answer[ "result" ] = result;
    return true;
}

bool Wifi::connect( const JsonObject& message, JsonObject& answer ) {
    std::string ssid, password;
    bool isOpen = false;
    bool isHidden = false;
    if( !getField( message, "ssid", answer, ssid ) )
        return false;
    ssid = quotes( ssid );
    std::vector< std::string > lines;
    json_get( message, "open", isOpen );
    json_get( message, "hidden", isHidden );
    if( json_get( message, "password", password ) || isOpen ) {
        if( !password.empty() )
            password = quotes( password );
        if( !executeScript( "wf-cmd.sh remove " + ssid, lines, answer ) ) {
            Log::error( "Failed to run \"wf-cmd.sh remove " + ssid + "\"" );
            return false;
        }
        if( lines.back() != "OK" ) {
            setError( answer, Errors::REMOVE_NET_ERROR );
            return false;
        }
        if( !executeScript( "wf-cmd.sh " + std::string( isHidden ? "add_hidden " : "add " ) + ssid + " " + password,
                            lines,
                            answer ) )
            return false;
        if( lines.back() != "OK" ) {
            setError( answer, Errors::ADD_NET_ERROR );
            return false;
        }
    }
    if( !executeScript( "wf-cmd.sh connect " + ssid, lines, answer ) )
        return false;
    return true;
}

bool Wifi::del( const JsonObject& message, JsonObject& answer ) {
    std::string ssid;
    if( !getField( message, "ssid", answer, ssid ) )
        return false;
    ssid = quotes( ssid );
    std::vector< std::string > lines;
    if( !executeScript( "wf-cmd.sh remove " + ssid, lines, answer ) )
        return false;
    if( lines.back() != "OK" ) {
        setError( answer, Errors::REMOVE_NET_ERROR );
        return false;
    }
    return true;
}

bool Wifi::getStatus( const JsonObject& message, JsonObject& answer ) {
    std::string ssid;
    if( !getField( message, "ssid", answer, ssid ) )
        return false;
    ssid = quotes( ssid );
    std::vector< std::string > lines;
    if( !executeScript( "wf-cmd.sh check " + ssid, lines, answer ) )
        return false;
    if( lines.size() < 1 ) {
        setError( answer, Errors::CHECK_ERROR );
    }
    JsonObject result;
    parseJson( lines.front(), result );
    answer[ "result" ] = result;
    return true;
}

bool Wifi::reset( JsonObject& answer ) {
    std::vector< std::string > lines;
    if( !executeScript( "wf-cmd.sh reset_config", lines, answer ) )
        return false;
    if( lines.back() != "OK" ) {
        setError( answer, Errors::RESET_WIFI_ERROR );
        return false;
    }
    return true;
}

bool Wifi::getIp( JsonObject& answer ) {
    std::vector< std::string > lines;
    JsonObject jsonLine;
    if( !executeScript( "wf-cmd.sh info", lines, answer ) )
        return false;
    if( !lines.empty() )
        parseJson( lines.front(), jsonLine );
    answer[ "result" ] = jsonLine;
    return true;
}

void Wifi::run( RunFunction f, RunCallBack callBack, JsonObject& answer ) {
    static std::mutex m;
    std::thread( runThread, &m, f, callBack, answer ).detach();
}

void Wifi::runThread( std::mutex* m, RunFunction f, RunCallBack callBack, JsonObject answer ) {
    m->lock();
    f( answer );
    callBack( json_write( answer ) );
    m->unlock();
}
