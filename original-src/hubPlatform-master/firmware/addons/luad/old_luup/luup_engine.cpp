#include "luad/old_luup/luup_engine.h"

namespace HubPlatform {

class DeviceError : public std::runtime_error {
public:
    DeviceError( const std::string& errDesription )
    : runtime_error( "fail on a device: " + errDesription ) {}
};

Device::Device( int deviceID, int sock )
: m_deviceID{ deviceID }
, m_connectionSocket{ sock }
, m_interceptionMode{ InterceptionMode::Intercept } {}

Device::~Device() {
    flushDeviceUpdates();
}

Device Device::makeDevice( int deviceID ) {
    return Device{ deviceID, -1 };
}

void Device::setConnectionSocket( int sock ) {
    m_connectionSocket = sock;
}

void Device::setInterceptionMode( InterceptionMode mode ) {
    m_interceptionMode = mode;
}

bool Device::addAsyncLuaTask( AsyncLuaTask task ) {
    auto taskID = task.getID();
    auto res = m_asyncLuaTasks.emplace( taskID, std::move( task ) );
    return res.second == true;
}

AsyncLuaTask& Device::getAsyncLuaTask( AsyncTaskID taskID ) {
    auto it = m_asyncLuaTasks.find( taskID );
    if( it != m_asyncLuaTasks.end() )
        return ( *it ).second;

    throw DeviceError{ "failed to find an async job for the device" };
}

const AsyncLuaTask& Device::getAsyncLuaTask( AsyncTaskID taskID ) const {
    auto it = m_asyncLuaTasks.find( taskID );
    if( it != m_asyncLuaTasks.end() )
        return ( *it ).second;

    throw DeviceError{ "failed to find an async job for the device" };
}

void Device::flushDeviceUpdates() {}

}  // namespace HubPlatform
