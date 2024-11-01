#include "webtrc_group_info.h"
#include <regex>

namespace HubPlatform {
namespace CoreD {

WebRtcGroupInfo::WebRtcGroupInfo() {
    pipePtr.reset(
        popen( "cgget -g memory:webrtc | grep -E 'memory\\.usage_in_bytes|memory\\.limit_in_bytes|oom_kill'", "r" ) );
    if( !pipePtr ) {
        LOGGER_ERROR( "Error executing command: cgget " );
        return;
    }

    char buffer[ 128 ];
    while( fgets( buffer, sizeof( buffer ), pipePtr.get() ) != nullptr ) {
        m_groupInfo << buffer;
    }
}

Json WebRtcGroupInfo::getMemoryLimitAndUsage() {
    try {
        std::regex pattern( "(memory\\.limit_in_bytes|memory\\.usage_in_bytes):\\s*(\\S+)" );
        std::smatch match;
        Json jsonResult = JSON_EMPTY_OBJ;

        auto findString = m_groupInfo.str();
        while( std::regex_search( findString, match, pattern ) ) {
            std::string key = match[ 1 ].str();
            std::string value = match[ 2 ].str();
            if( key == "memory.limit_in_bytes" ) {
                key = "memoryLimitInBytes";
            }
            else if( key == "memory.usage_in_bytes" ) {
                key = "memoryUsedInBytes";
            }
            jsonResult[ key ] = value;
            findString = match.suffix();
        }

        return jsonResult;
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( "Get webrtc group memory limits error" );
        return JSON_EMPTY_OBJ;
    }
}

Json WebRtcGroupInfo::getOomControl() {
    try {
        std::string outputString = m_groupInfo.str();
        std::regex pattern( "memory\\.oom_control: oom_kill_disable (\\d+)[\\s\\S]+oom_kill (\\d+)" );
        std::smatch match;
        Json jsonResult = JSON_EMPTY_OBJ;

        if( std::regex_search( outputString, match, pattern ) ) {
            int disabled = std::stoi( match[ 1 ].str() );
            int kills = std::stoi( match[ 2 ].str() );

            jsonResult[ "disabled" ] = disabled;
            jsonResult[ "kills" ] = kills;
        }

        return jsonResult;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "Get webrtc group memory limits error" );
        return JSON_EMPTY_OBJ;
    }
}

}  // namespace CoreD
}  // namespace HubPlatform
