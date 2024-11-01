#include "bluetooth/proxies/gatt_service_proxy.h"
#include "gatt_service_proxy.h"

namespace HubPlatform {

GattServiceProxy::GattServiceProxy( GDBusProxy* proxy )
: GattAttributeProxy( proxy ) {}

std::string GattServiceProxy::getDevicePath() const {
    return getProperty< std::string >( "Device" );
}

}  // namespace HubPlatform
