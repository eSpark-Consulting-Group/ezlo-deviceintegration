#include "addon_interfaces/addon.h"
#include "service_interfaces/bus_wrapper.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace Addons {

const char* const AddonsPrefix = "addon.";
const char* const AddonPingMethod = "pingAddon";

Addon::Addon( const std::string& name )
: Service( name )
, m_qualifiedName( createQualifiedName( name ) ) {}

Addon::~Addon() {}

Addon::Addon( const std::string& name,
              std::unique_ptr< Services::BusWrapper > busWrapper,
              std::unique_ptr< IDbClient > dbClient )
: Service( name, std::move( busWrapper ), std::move( dbClient ) )
, m_qualifiedName( AddonsPrefix + name ) {}

void Addon::setUpFinished() {
    m_setupId = std::chrono::steady_clock::now().time_since_epoch().count();

    sendAddonStartedBroadcast();
}

void Addon::tearDown() {
    JsonObject addonId = JSON( "id" << getName() << "version" << getVersion().toString() );
    Services::Message message = { "extensions_manager", EVENT, ADDON_STOPPED, addonId };
    send( message );

    Service::tearDown();
}

void Addon::handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) {
    const auto method = json_get_str( json[ "method" ] );
    if( method == AddonPingMethod ) {
        sendAddonStartedBroadcast();
    }
    else {
        Service::handleLegacyMethodCall( json, sender );
    }
}

void Addon::sendAddonStartedBroadcast() {
    JsonObject addonSummary =
        JSON( "id" << getName() << "version" << getVersion().toString() << "setupId" << m_setupId );
    Services::Message message = { "extensions_manager", EVENT, ADDON_STARTED, addonSummary };
    send( message );
}

const std::string& Addon::getQualifiedName() const {
    return m_qualifiedName;
}

std::string Addon::createQualifiedName( const std::string& name ) {
    return AddonsPrefix + name;
}

}  // namespace Addons
}  // namespace HubPlatform
