
#include "device_model/h/apiclasses.h"
#include "device_model/h/device.h"

#include "database_client/dbid.h"

namespace fo {

Device::Device() {}

Device::Device( const std::string& name,
                const std::string& type,
                const std::string& subType,
                const DeviceType& deviceType,
                const std::string& vendor,
                const std::string& model,
                const std::string& description,
                const bool batteryPowered,
                const bool hasWakeup,
                const bool reachable,
                const bool ready,
                const bool complex,
                const bool hasSecurity,
                const bool secureChannelEstablished,
                const bool persistent )
: m_deviceId( db::DbId::genOID() )
, m_name( name )
, m_type( type )
, m_deviceSubType( subType )
, m_deviceType( deviceType )
, m_vendor( vendor )
, m_model( model )
, m_description( description )
, m_batteryPowered( batteryPowered )
, m_hasWakeup( hasWakeup )
, m_reachable( reachable )
, m_ready( ready )
, m_complex( complex )
, m_hasSecurity( hasSecurity )
, m_secureChannelEstablished( secureChannelEstablished )
, m_persistent( persistent ) {}

const std::string& Device::getDbId() const {
    return m_deviceId;
}

const std::string& Device::getName() const {
    return m_name;
}

const std::string& Device::getType() const {
    return m_type;
}

const std::string& Device::getSubType() const {
    return m_deviceSubType;
}

DeviceType Device::getDeviceType() const {
    return m_deviceType;
}

bool Device::getBatteryPowered() const {
    return m_batteryPowered;
}

bool Device::getHasWakeup() const {
    return m_hasWakeup;
}

bool Device::getReachable() const {
    return m_reachable;
}

bool Device::getReady() const {
    return m_ready;
}

bool Device::getComplex() const {
    return m_complex;
}

bool Device::getHasSecurity() const {
    return m_hasSecurity;
}

bool Device::getSecureChannelEstablished() const {
    return m_secureChannelEstablished;
}

const std::string& Device::getVendor() const {
    return m_vendor;
}

const std::string& Device::getModel() const {
    return m_model;
}

const std::string& Device::getDescription() const {
    return m_description;
}

void Device::setDeviceId( const std::string& deviceId ) {
    m_deviceId = deviceId;
}

bool Device::getPersistent() const {
    return m_persistent;
}

}  // namespace fo
