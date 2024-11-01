#include "extensions_manager/extensions/extension_dependency.h"
#include "plugin_utils/plugin_summary.h"
#include "common_utils/addons/addon_summary.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace ExtensionsManager {

template < class SummaryType >
ExtensionDependency< SummaryType > ExtensionDependency< SummaryType >::fromJson( const JsonObject& config ) {
    if( config.find( "id" ) == config.end() )
        return {};

    if( config.find( "version" ) == config.end() )
        return {};

    return { json_get_str( config[ "id" ] ), RequiredVersion( json_get_str( config[ "version" ] ) ) };
}

template < class SummaryType >
std::string ExtensionDependency< SummaryType >::toString() const {
    return id + "-" + version.toString();
}

template < class SummaryType >
bool ExtensionDependency< SummaryType >::isValid() const {
    return !id.empty() && version.isValid();
}

template <>
bool ExtensionDependency< CommonUtils::AddonSummary >::isSatisfiedBy(
    const CommonUtils::AddonSummary& addonSummary ) const {
    return ( id == addonSummary.id ) && ( version.getMajor() == addonSummary.version.getMajor() ) &&
           ( version.getMinor() <= addonSummary.version.getMinor() );
}

template <>
bool ExtensionDependency< CommonUtils::PluginSummary >::isSatisfiedBy(
    const CommonUtils::PluginSummary& pluginSummary ) const {
    return ( id == pluginSummary.id ) && ( version.getMajor() == pluginSummary.version.getMajor() ) &&
           ( version.getMinor() <= pluginSummary.version.getMinor() );
}

template < class SummaryType >
bool ExtensionDependency< SummaryType >::operator==( const ExtensionDependency& rhs ) const {
    return ( id == rhs.id ) && ( version == rhs.version );
}

template class ExtensionDependency< CommonUtils::AddonSummary >;
template class ExtensionDependency< CommonUtils::PluginSummary >;

}  // namespace ExtensionsManager
}  // namespace HubPlatform
