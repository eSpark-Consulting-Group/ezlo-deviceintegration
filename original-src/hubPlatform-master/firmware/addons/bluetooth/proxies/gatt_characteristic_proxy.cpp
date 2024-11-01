#include "bluetooth/proxies/gatt_characteristic_proxy.h"

#include "bluetooth/proxy_method_helpers/write_characteristic_helper.h"
#include "bluetooth/proxy_method_helpers/read_characteristic_helper.h"
#include "bluetooth/proxy_method_helpers/start_notify_helper.h"
#include "bluetooth/proxy_method_helpers/stop_notify_helper.h"

#include <algorithm>

namespace HubPlatform {

const char* GattCharacteristicProxy::NotifyFlag = "notify";
const char* GattCharacteristicProxy::IndicateFlag = "indicate";
const char* GattCharacteristicProxy::ValuePropertyName = "Value";

GattCharacteristicProxy::GattCharacteristicProxy( GDBusProxy* proxy )
: GattAttributeProxy( proxy ) {}

void GattCharacteristicProxy::startNotify() {
    StartNotifyHelper( *this ).callProxyMethod();
}

void GattCharacteristicProxy::stopNotify() {
    StopNotifyHelper( *this ).callProxyMethod();
}

std::string GattCharacteristicProxy::getServicePath() const {
    return getProperty< std::string >( "Service" );
}

std::vector< std::string > GattCharacteristicProxy::getFlags() const {
    return getArrayProperty< std::string >( "Flags" );
}

std::vector< std::uint8_t > GattCharacteristicProxy::getCachedValue() const {
    return getArrayProperty< std::uint8_t >( ValuePropertyName );
}

void GattCharacteristicProxy::writeValue( const std::vector< std::uint8_t >& data ) {
    WriteCharacteristicHelper( *this, data ).callProxyMethod();
}

std::vector< std::uint8_t > GattCharacteristicProxy::readValue() {
    ReadCharacteristicHelper helper( *this );
    helper.callProxyMethod();
    return helper.getResult();
}

bool GattCharacteristicProxy::isNotificationSupported() const {
    static const char* const notificationFlags[] = { NotifyFlag, IndicateFlag };

    const auto flags = getFlags();
    return std::search( std::begin( flags ),
                        std::end( flags ),
                        std::begin( notificationFlags ),
                        std::end( notificationFlags ) ) != std::end( flags );
}

}  // namespace HubPlatform
