#pragma once
#include "json_utils/json_path.h"
class IDbClient;
namespace HubPlatform {
namespace CoreD {
namespace Protect {
class ProtectValidator {
public:
    explicit ProtectValidator( IDbClient& client )
    : m_dbClient( client ) {}

    JsonObject validateAddContactIdAliasRequest( const JsonObject& message );
    JsonObject validateAddSecurityConfigRequest( const JsonObject& message );

private:
    IDbClient& m_dbClient;
};
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform
