#include "networkd/configuration/nm_chat_script.h"

#include "json_utils/json_utils.h"

#include <sstream>
#include <fstream>

namespace HubPlatform {
namespace Network {

namespace ChatOption {
extern const char* const PDPType = "PDPType";
}  // namespace ChatOption

namespace {

std::string getOption( const Json& config, const char* const key, const char* const defaultValue ) {
    auto option = json_get_str( config, key );
    if( !option.empty() )
        return option;
    return ( defaultValue ) ? defaultValue : "";
}

bool saveFile( const std::string& fileName, const std::string& settings ) {
    std::ofstream stream( fileName );
    if( !stream.is_open() )
        return false;

    stream << settings;
    stream.flush();
    return true;
}

}  // namespace

bool ChatScript3G::saveScript( const Json& config ) {
    std::string pdpType = getOption( config, ChatOption::PDPType, "IP" );

    std::stringstream ss;
    ss << "ABORT   BUSY\n";
    ss << "ABORT   'NO CARRIER'\n";
    ss << "ABORT   ERROR\n";
    ss << "REPORT  CONNECT\n";
    ss << "TIMEOUT 10\n";
    ss << "\"\"      \"AT&F\"\n";
    ss << "OK      \"ATE0\"\n";
    ss << "OK      'AT+CGDCONT=1,\"" << pdpType << "\",\"$USE_APN\"'\n";
    ss << "SAY     \"Calling UMTS/GPRS\"\n";
    ss << "TIMEOUT 30\n";
    ss << "OK      \"ATD$DIALNUMBER\"\n";
    ss << "CONNECT ' '";

    return saveFile( "/etc/chatscripts/3g.chat", ss.str() );
}

}  // namespace Network
}  // namespace HubPlatform
