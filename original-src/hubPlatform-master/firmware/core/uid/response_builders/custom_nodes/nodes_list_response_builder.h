#include <string>
#include <set>
#include <json_utils/json_utils.h>

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace Uid {
namespace CustomNodes {

class NodesListResponseBuilder {
public:
    NodesListResponseBuilder( Services::Service& service );
    ~NodesListResponseBuilder() = default;

    JsonObject getDataWithFilter();
    void setAuthorFilteringParam( const std::string& author );
    void setPluginName( const std::string& pluginName );
    void setPluginShortName( const std::string& shortName );
    void setShowMeta( bool showMeta );
    void setShowAdvanced( bool showAdvanced );
    void setShowInterface( bool showInterface );
    void setIds( std::set< std::string > ids );

private:
    JsonObject extractMetadataFromItem( const JsonObject& item );
    JsonObject extractConfigurationFromItem( const JsonObject& item );
    JsonObject extractInterfaceContent( const std::string& nodeName );

private:
    Services::Service& m_service;
    std::string m_author;
    std::string m_pluginName;
    std::string m_pluginShortName;
    bool m_showMeta{ true };
    bool m_showAdvanced{ false };
    bool m_showInterface{ false };
    std::set< std::string > m_ids;
};

}  // namespace CustomNodes
}  // namespace Uid
}  // namespace HubPlatform
