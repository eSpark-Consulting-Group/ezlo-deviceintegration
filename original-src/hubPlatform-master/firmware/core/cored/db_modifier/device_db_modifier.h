#pragma once

#include "cored/db_modifier/db_modifier.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "bus/messagesclasses.h"
#include "cored/what_happened.h"

#include <string>
#include <functional>
#include <atomic>
#include <set>

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class DeviceDbModifier final : public DbModifier {
public:
    DeviceDbModifier( IDbClient& db, Broadcaster broadcaster );
    ~DeviceDbModifier() override = default;

    bool hasChanges() const override;
    void setChangedStatus( bool isChanged ) override;
    Json& prepareInsertBroadcastMessage( Json& broadcastData ) override;
    Json& prepareUpdateBroadcastMessage( Json& broadcastData );
    Json& prepareRemoveBroadcastMessage( Json& broadcastData, const Json& removedRecord ) override;
    bool isUpdateBroadcastEmpty( const Json& broadcastData ) const override;

    void sendUpdateBroadcast( MessageSubClasses subClass, Json& broadcastData, const Json& oldFieldsValues ) override;

private:
    bool checkProtectFeatureStatus();
    bool isUsingProtectConfig() override final;

private:
    std::atomic_bool m_changedStatus{ false };
    std::set< std::string > m_excludeBroadcastKeys;
};

}  // namespace CoreD
}  // namespace HubPlatform
