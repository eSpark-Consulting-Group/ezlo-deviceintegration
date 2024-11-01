#include "json_utils/json_fwd.h"
#include "protect.h"
#include "database_client/idbclient.h"
namespace HubPlatform {
namespace CoreD {
namespace Protect {

class SecurityConfigInfo : public ProtectInfo {
public:
    SecurityConfigInfo() = default;
    Json toJson() const override;
    bool fromJson( const JsonObject& data ) override;
    bool getAlarmFlag() const;
    std::string getSoundType() const;
    std::string getChime() const;
    JsonObject getSignalObject() const;
    JsonArray getArmedWhen() const;
    void buildSignalField( const JsonObject& signal,
                           const std::string& signalField,
                           IDbClient& dbClient,
                           Json& result );
    std::string getEntryDelay() const;

private:
    std::string m_id;
    std::string m_name;
    std::string m_description;
    bool m_alarm{ false };
    std::string m_soundType;
    std::string m_entryDelay;
    std::string m_chime;
    std::string m_follower;
    JsonArray m_armedWhen;
    JsonArray m_recommendCategories;
    JsonObject m_signal;
    bool m_persistent{ false };
};
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform