#pragma once

#include "json_utils/json_fwd.h"

class IDbClient;

namespace HubPlatform {
namespace Notifications {

class DatabaseSettings {
public:
    DatabaseSettings( IDbClient& dbClient );

    void setSMTPSettings( JsonObject settings );
    JsonObject getSMTPSettings();
    JsonObject getHashedSMTPSettings();  // Sensitive data replaced with hashes

private:
    IDbClient& m_dbClient;
};

}  // namespace Notifications
}  // namespace HubPlatform
