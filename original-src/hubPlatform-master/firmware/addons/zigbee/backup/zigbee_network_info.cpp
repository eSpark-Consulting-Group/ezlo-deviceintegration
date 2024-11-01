#include "zigbee/backup/zigbee_network_info.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {
namespace Backup {

namespace {

template < typename T >
JsonArray arrayToJson( const T* arr, int size ) {
    JsonArray result;
    for( int i = 0; i < size; ++i ) {
        result.push_back( arr[ i ] );
    }

    return result;
}

template < typename T >
void jsonToArray( Json source, T* arr, int size ) {
    for( int i = 0; i < size; ++i ) {
        arr[ i ] = source[ i ].get< T >();
    }
}

}  // namespace

Json networkInfoToJson( const ZigbeeNetworkInfo& info ) {
    Json result;

    result[ Resources::Fields::NetworkInfo::NodeId ] = info.nodeId;
    result[ Resources::Fields::NetworkInfo::PanId ] = info.panId;
    result[ Resources::Fields::NetworkInfo::RadioPower ] = info.radioPower;
    result[ Resources::Fields::NetworkInfo::RadioChannel ] = info.radioChannel;
    result[ Resources::Fields::NetworkInfo::EpanId ] = arrayToJson( info.ePanId, 8 );
    result[ Resources::Fields::NetworkInfo::Euid64 ] = arrayToJson( info.eui64, EUI64_SIZE );
    result[ Resources::Fields::NetworkInfo::GlobalKey ] =
        arrayToJson( info.globalKey.contents, EMBER_ENCRYPTION_KEY_SIZE );
    result[ Resources::Fields::NetworkInfo::GlobalFc ] = info.globalFC;
    result[ Resources::Fields::NetworkInfo::NetworkKey ] =
        arrayToJson( info.networkKey.contents, EMBER_ENCRYPTION_KEY_SIZE );
    result[ Resources::Fields::NetworkInfo::NetworkFc ] = info.networkFC;
    result[ Resources::Fields::NetworkInfo::NetworkKeySeqNumber ] = info.networkKeySeqNum;

    return result;
}

ZigbeeNetworkInfo networkInfoFromJson( const Json& info ) {
    ZigbeeNetworkInfo result;

    result.nodeId = info[ Resources::Fields::NetworkInfo::NodeId ].get< int8u >();
    result.panId = info[ Resources::Fields::NetworkInfo::PanId ].get< int16u >();
    result.radioPower = info[ Resources::Fields::NetworkInfo::RadioPower ].get< int8s >();
    result.radioChannel = info[ Resources::Fields::NetworkInfo::RadioChannel ].get< int8u >();
    jsonToArray( info[ Resources::Fields::NetworkInfo::EpanId ], result.ePanId, 8 );
    jsonToArray( info[ Resources::Fields::NetworkInfo::Euid64 ], result.eui64, EUI64_SIZE );
    jsonToArray(
        info[ Resources::Fields::NetworkInfo::GlobalKey ], result.globalKey.contents, EMBER_ENCRYPTION_KEY_SIZE );
    result.globalFC = info[ Resources::Fields::NetworkInfo::GlobalFc ].get< int32u >();
    jsonToArray(
        info[ Resources::Fields::NetworkInfo::NetworkKey ], result.networkKey.contents, EMBER_ENCRYPTION_KEY_SIZE );
    result.networkFC = info[ Resources::Fields::NetworkInfo::NetworkFc ].get< int32u >();
    result.networkKeySeqNum = info[ Resources::Fields::NetworkInfo::NetworkKeySeqNumber ].get< int8u >();

    return result;
}

}  // namespace Backup
}  // namespace Zigbee
}  // namespace HubPlatform
