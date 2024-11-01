#pragma once

#include <functional>
#include <znet/iznet_provisioning_list_provider.hpp>

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace Zwave {

class ProvisioningListProvider : public znet::IProvisioningListProvider {
public:
    ProvisioningListProvider( IDbClient& dbClient, const char* tableName );

    bool storeProvisioningListEntry( const znet::ProvisioningListEntry& ) override;
    bool removeProvisioningListEntryByID( uint32_t ) override;
    bool findProvisioningListEntryByID( uint32_t, znet::ProvisioningListEntry& ) const override;
    bool findProvisioningListEntryByNodeID( uint8_t, znet::ProvisioningListEntry& ) const;
    bool findProvisioningListEntryByDSK( const znet::DSK&, znet::ProvisioningListEntry& ) const override;
    size_t getProvisioningListEntryCount() const override;
    bool loadProvisioningList( const IProvisioningListProvider::ProcessProvisioningCallback& ) const override;

private:
    IDbClient& m_dbClient;
    const char* m_tableName;
};

}  // namespace Zwave
}  // namespace HubPlatform
