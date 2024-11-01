#include "json_utils/json_fwd.h"
#include "protect.h"
namespace HubPlatform {
namespace CoreD {
namespace Protect {

class ContactIdInfo : public ProtectInfo {
public:
    ContactIdInfo() = default;
    Json toJson() const override;
    bool fromJson( const JsonObject& data ) override;

private:
    std::string m_id;
    std::string m_text;
    std::string m_on;
    std::string m_off;
    bool m_persistent{ false };
};
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform