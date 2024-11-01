#include "zwave/provisioning/provisioning_list_provider.h"
#include "zwave/provisioning/provisioning_list_utils.h"
#include "zwave/resources/zwave_resources.h"

namespace HubPlatform {
namespace Zwave {

ProvisioningListProvider::ProvisioningListProvider( IDbClient& dbClient, const char* tableName )
: m_dbClient( dbClient )
, m_tableName( tableName ? tableName : "" ) {}

bool ProvisioningListProvider::storeProvisioningListEntry( const znet::ProvisioningListEntry& entry ) {
    Json json;
    ProvisioningListUtils::entryToJson( entry, json, true );
    m_dbClient.update( m_tableName, JSON_EMPTY_OBJ, json, true );
    return true;
}

bool ProvisioningListProvider::removeProvisioningListEntryByID( uint32_t provisioningId ) {
    m_dbClient.remove( m_tableName, JSON( "_id" << ProvisioningListUtils::idToString( provisioningId ) ) );
    return true;
}

bool ProvisioningListProvider::findProvisioningListEntryByID( uint32_t provisioningId,
                                                              znet::ProvisioningListEntry& entry ) const {
    auto json = m_dbClient.findOne(
        m_tableName, JSON( "_id" << ProvisioningListUtils::idToString( provisioningId ) ), nullptr );
    return ProvisioningListUtils::entryFromDbJson( json, entry );
}

bool ProvisioningListProvider::findProvisioningListEntryByNodeID( uint8_t nodeId,
                                                                  znet::ProvisioningListEntry& entry ) const {
    auto json = m_dbClient.findOne( m_tableName, JSON( "node" << nodeId ), nullptr );
    return ProvisioningListUtils::entryFromDbJson( json, entry );
}

bool ProvisioningListProvider::findProvisioningListEntryByDSK( const znet::DSK& dsk,
                                                               znet::ProvisioningListEntry& entry ) const {
    auto json = m_dbClient.findOne( m_tableName, JSON( "dsk" << ProvisioningListUtils::dskToString( dsk ) ), nullptr );

    return ProvisioningListUtils::entryFromDbJson( json, entry );
}

size_t ProvisioningListProvider::getProvisioningListEntryCount() const {
    return m_dbClient.count( m_tableName, JSON_EMPTY_OBJ );
}

bool ProvisioningListProvider::loadProvisioningList(
    const IProvisioningListProvider::ProcessProvisioningCallback& callback ) const {
    auto jsons = m_dbClient.findAll( m_tableName, JSON_EMPTY_OBJ, nullptr );

    for( auto& json : jsons ) {
        znet::ProvisioningListEntry entry;

        if( !ProvisioningListUtils::entryFromDbJson( json, entry ) )
            return false;

        if( callback( entry ) )
            return true;
    }
    return true;
}

}  // namespace Zwave
}  // namespace HubPlatform
