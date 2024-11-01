#pragma once

#include "jsonrpc/message.h"
#include <unordered_set>

namespace HubPlatform {
namespace Uid {

class HubObjectListDeviceFilter {
public:
    explicit HubObjectListDeviceFilter( rpc::Message const& rpcMessage );

    HubObjectListDeviceFilter( HubObjectListDeviceFilter const& ) = delete;

    HubObjectListDeviceFilter( HubObjectListDeviceFilter&& ) = delete;

    HubObjectListDeviceFilter& operator=( HubObjectListDeviceFilter const& ) = delete;

    HubObjectListDeviceFilter& operator=( HubObjectListDeviceFilter& ) = delete;

    ~HubObjectListDeviceFilter() = default;

    bool filterDeviceIds( const std::string& deviceId ) const;

protected:
    bool fastSearch( std::string const& searchKey ) const;

    size_t buildSearchMap();

private:
    rpc::Message const& m_rpcMessage;
    bool m_hasParams = false;
    JsonObject m_params;

    typedef std::unordered_set< std::string > StringSet;
    StringSet m_deviceIdStorage;
};

}  // namespace Uid
}  // namespace HubPlatform