#pragma once

#include <map>
#include <string>
#include <mutex>
#include <atomic>

namespace HubPlatform {

/* @brief This class represents an async task (a job in the old luup) created by a luup API 'luup.call_action' call,
 * it holds all information needed to run, wait for incoming data and other book-keeping info */
class AsyncLuaTask final {
public:
    enum class Status {
        None = -1,             // old_luup::jobNone
        WaitingToStart,        // old_luup::jobWaitingToStart
        InProgress,            // old_luup::jobInProgress
        Error,                 // old_luup::jobError
        Aborted,               // old_luup::jobAborted
        Done,                  // old_luup::jobDone
        WaitingForCallback,    // old_luup::jobWaitingForCallback
        Requeue,               // old_luup::jobRequeue
        InProgressPendingData  // old_luup::jobInProgressPendingData
    };

    using Setting = std::string;
    using Value = std::string;

    AsyncLuaTask();

    /* async lua task is a unique stuff... */
    AsyncLuaTask( const AsyncLuaTask& rhs ) = delete;
    AsyncLuaTask& operator=( const AsyncLuaTask& rhs ) = delete;

    AsyncLuaTask( AsyncLuaTask&& rhs );
    AsyncLuaTask& operator=( AsyncLuaTask&& rhs ) = delete; /* TODO: do we really need such a functionality? */

    bool isValid() const {
        return m_status != Status::None;
    }

    int getID() const {
        return m_id;
    }
    int getDeviceID() const {
        return m_deviceID;
    }
    Status getStatus() const {
        return m_status;
    }
    std::string getNotes() const;

    bool setSetting( const Setting& setting, const Value& value );
    Value getSettingValue( const Setting& setting );

private:
    std::mutex m_mtx;

    const int m_id;
    const int m_deviceID;           /* a device this task belongs to */
    std::atomic< Status > m_status; /* I'm not sure if C++ generates a real lock-free code even for std::atomic_int,
                                       not to mention something not trivial or something not obvious, like an
                                       enumeration type, but why not to try to transfer synchronize headache to a
                                       compiler (at least for non-complex types) even if there's no lock-free at all? */

    using Settings = std::map< Setting, Value >;
    Settings m_settings;
};

}  // namespace HubPlatform
