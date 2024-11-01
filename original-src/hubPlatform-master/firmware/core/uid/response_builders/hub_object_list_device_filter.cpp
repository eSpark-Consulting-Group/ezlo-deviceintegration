#include "hub_object_list_device_filter.h"
#include "cored/model/resources/cored_resources.h"
#include <algorithm>

namespace HubPlatform {
namespace Uid {

const std::string DEVICE_IDS = "deviceIds";

HubObjectListDeviceFilter::HubObjectListDeviceFilter( rpc::Message const& rpcMessage )
: m_rpcMessage( rpcMessage ) {
    m_hasParams = m_rpcMessage.getParams( m_params );
    buildSearchMap();
}

bool HubObjectListDeviceFilter::filterDeviceIds( const std::string& deviceId ) const {
    if( !m_hasParams )
        return true;

    return fastSearch( deviceId );
}

bool HubObjectListDeviceFilter::fastSearch( std::string const& searchKey ) const {
    if( m_deviceIdStorage.empty() )
        return true;
    return m_deviceIdStorage.find( searchKey ) != end( m_deviceIdStorage );
}

size_t HubObjectListDeviceFilter::buildSearchMap() {
    if( !m_hasParams || m_params.count( DEVICE_IDS ) == 0 || !m_params[ DEVICE_IDS ].is_array() )
        return 0;

    auto const& jsonDevices = m_params[ DEVICE_IDS ];

    std::for_each( begin( jsonDevices ), end( jsonDevices ), [this]( JsonObject const& deviceId ) {
        std::string const& str = deviceId.get< std::string >();
        this->m_deviceIdStorage.insert( str );
    } );
    return m_deviceIdStorage.size();
}

}  // namespace Uid
}  // namespace HubPlatform
