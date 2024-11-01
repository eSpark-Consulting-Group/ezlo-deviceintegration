#pragma once
#include <tiny/tiny_consolecolors_filters.h>

namespace HubPlatform {
namespace Uid {

class WsCommandRpcMessageFilters : public cc::IRpcMessageFilters {
public:
    WsCommandRpcMessageFilters();
    enum Limits { SIZE_LIMIT = 2048 };
    size_t sizeLimit() const override;
    void sizeLimit( size_t limit ) override;
    ;

    bool needFiltering( const std::string& data ) const override;
    /**
     *  apply filters on 'data'
     *  if a pattern is found, convert string to json.
     *       apply truncation actions
     *  else
     *       just truncate raw data
     */
    bool updateJsonDump( const std::string& data, std::string& truncated ) override;

private:
    size_t m_limit;

    std::string truncateString( const std::string& str ) const;
};

}  // namespace Uid
}  // namespace HubPlatform