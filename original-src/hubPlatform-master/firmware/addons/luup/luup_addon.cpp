#include "luup_addon.h"
#include "version/luup_addon_version.h"

#include "upnp_service.h"
#include "mongoose_service.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Addons {

const char* const LuupAddon::Name = "luup";

LuupAddon::LuupAddon( const std::string& webRoot )
: Addon( Name )
, m_webRoot( webRoot ) {}

LuupAddon::~LuupAddon() {}

CommonUtils::AddonVersion LuupAddon::getVersion() const {
    return Addons::Luup::getVersion();
}

void LuupAddon::setUp() {
    Addon::setUp();

#ifdef USE_UPNP_SERVICE
    m_services.emplace_back( std::make_unique< UPnPService >( *this, m_webRoot ) );
#endif
    m_services.emplace_back( std::make_unique< MongooseService >( *this, m_webRoot ) );

    addTask( [this]() {
        for( auto& service : m_services ) {
            service->start();
        }
    } );
}

}  // namespace Addons
}  // namespace HubPlatform
