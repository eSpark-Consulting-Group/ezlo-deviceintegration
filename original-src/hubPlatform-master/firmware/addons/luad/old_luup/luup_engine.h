#pragma once

#include "luad/old_luup/async_lua_task.h"

#include <map>

namespace HubPlatform {

/* @brief represents a device in the database
 *
 * m_DeviceID - a device id the representation is for
 * m_ConnectionSocket - a socket fd which refers a remote connection owned by this device
 * m_InterceptionMode - should Luup engine consume data on a connection socket or not,
 *                      by default Luup engine consumes (intercepts) data
 *
 * @note this representation maps several fields from a database' entry for a device,
 * so not all information is used here, only necessary for the luad
 *
 * @note this is only representation, it doesn't own any resources
 */
class Device final {
public:
    enum class InterceptionMode { Pass, Intercept };

    using AsyncTaskID = int;

    ~Device();

    Device( const Device& rhs ) = default;
    Device& operator=( const Device& rhs ) = default;

    Device( Device&& rhs ) = default;
    Device& operator=( Device&& rhs ) = default;

    static Device makeDevice( int deviceID );

    bool isConnectionEstablished() const {
        return m_connectionSocket >= 0;
    }
    int getConnectionSocket() const {
        return m_connectionSocket;
    }
    InterceptionMode getInterceptionMode() const {
        return m_interceptionMode;
    }

    void setConnectionSocket( int sock );
    void setInterceptionMode( InterceptionMode mode = InterceptionMode::Intercept );

    bool addAsyncLuaTask( AsyncLuaTask task );
    AsyncLuaTask& getAsyncLuaTask( AsyncTaskID taskID );
    const AsyncLuaTask& getAsyncLuaTask( AsyncTaskID taskID ) const;

private:
    Device( int deviceID, int sock );

    void flushDeviceUpdates();

private:
    int m_deviceID;
    int m_connectionSocket; /* TODO: this fd is valid only in a process which created it... */
    InterceptionMode m_interceptionMode;

    using DeviceLuaTasks = std::map< AsyncTaskID, AsyncLuaTask >;
    DeviceLuaTasks m_asyncLuaTasks;
};

}  // namespace HubPlatform
