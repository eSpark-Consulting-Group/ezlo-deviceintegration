#include <vector>
#include "rpc_message_filters.h"
#include "json_utils/json_fwd.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Uid {

WsCommandRpcMessageFilters::WsCommandRpcMessageFilters()
: m_limit( SIZE_LIMIT ) {}

size_t WsCommandRpcMessageFilters::sizeLimit() const {
    return m_limit;
}

void WsCommandRpcMessageFilters::sizeLimit( size_t limit ) {
    m_limit = limit;
}

namespace {
const std::vector< std::string >& quickFilters() {
    static const std::vector< std::string > quickFilters = { "hub.videoforward" };
    return quickFilters;
}
}  // namespace

bool WsCommandRpcMessageFilters::needFiltering( const std::string& data ) const {
    return data.size() > sizeLimit();
}

bool WsCommandRpcMessageFilters::updateJsonDump( const std::string& data, std::string& truncated ) {
    auto& filters = quickFilters();
    for( auto& filter : filters ) {
        auto pos = data.find( filter );
        if( std::string::npos != pos ) {
            if( filter == "hub.videoforward" ) {
                Json parsed;
                if( !parseJson( data, parsed ) )
                    continue;

                // type = offer
                Json sdp, actionReply;
                if( json_get( parsed, { "params", "params", "sdp" }, sdp ) ) {
                    if( sdp.is_string() ) {
                        const auto str = json_get_str( sdp );
                        parsed[ "params" ][ "params" ][ "sdp" ] = truncateString( str );
                    }
                }
                else if( json_get( parsed, { "result", "action_reply" }, actionReply ) ) {
                    if( actionReply.is_string() ) {
                        const auto str = json_get_str( actionReply );
                        parsed[ "result" ][ "action_reply" ] = truncateString( str );
                    }
                }
                if( !parsed.empty() ) {
                    truncated = parsed.dump();
                    return true;
                }
            }
        }
    }

    return false;
}

std::string WsCommandRpcMessageFilters::truncateString( const std::string& str ) const {
    return str.substr( 0, sizeLimit() ) + " (truncated, size: " + std::to_string( str.size() ) + ")";
}

}  // namespace Uid
}  // namespace HubPlatform
