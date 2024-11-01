#include "luad/old_luup/async_lua_task.h"

namespace HubPlatform {

AsyncLuaTask::AsyncLuaTask()
: m_id( -1 )
, m_deviceID( -1 )
, m_status( Status::None ) {}

AsyncLuaTask::AsyncLuaTask( AsyncLuaTask&& rhs )
: m_id( rhs.m_id )
, m_deviceID( rhs.m_deviceID ) {
    std::lock_guard< std::mutex > lk( rhs.m_mtx );

    m_status = rhs.m_status.load();
    m_settings = std::move( rhs.m_settings );

    rhs.m_status = Status::None;
}

std::string AsyncLuaTask::getNotes() const {
    return "wassup cruel world"; /* FIXME: ... */
}

bool AsyncLuaTask::setSetting( const Setting& setting, const Value& value ) {
    std::lock_guard< std::mutex > lk( m_mtx );

    auto res = m_settings.emplace( setting, value );
    return res.second == true;
}

AsyncLuaTask::Value AsyncLuaTask::getSettingValue( const Setting& setting ) {
    std::lock_guard< std::mutex > lk( m_mtx );

    auto it = m_settings.find( setting );
    if( it != m_settings.end() )
        return ( *it ).second;

    return Value{};
}

}  // namespace HubPlatform
