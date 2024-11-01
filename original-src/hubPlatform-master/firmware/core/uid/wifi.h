#pragma once

#include "json_utils/json_wrapper.h"

#include <string>
#include <mutex>
#include <functional>

class Wifi {
public:
    typedef std::function< bool( JsonObject& ) > RunFunction;
    typedef std::function< void( std::string ) > RunCallBack;
    static bool getList( JsonObject& answer );
    static bool connect( const JsonObject& message, JsonObject& answer );
    static bool del( const JsonObject& message, JsonObject& answer );
    static bool getStatus( const JsonObject& message, JsonObject& answer );
    static bool reset( JsonObject& answer );
    static bool getIp( JsonObject& answer );
    static void run( RunFunction f, RunCallBack callBack, JsonObject& answer );

private:
    static void runThread( std::mutex* m, RunFunction f, RunCallBack callBack, JsonObject answer );
    static bool executeScript( const std::string& name, std::vector< std::string >& result, JsonObject& answer );
};
