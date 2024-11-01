#pragma once

#include "common_utils/hub_state.h"

#include <functional>
#include <string>

namespace HubPlatform {
namespace BackupUpdateManager {

/*

Possible callback flows:
    start(...) == true;
        <none>

    start(...) == false;
        <- unlockedCallback(); // Either after service crash or firmware restart

    lock() == false;
        <none>

    lock() == true;
        <- lockedCallback( true );
        or
        <- lockedCallback( false );
        <- unlockedCallback(); or <firmware restart>

    isLocked() == true;
        <- serviceRestartedWhileLockedCallback();
        <- unlockedCallback(); or <firmware restart>

    unlock() == false; // No lock operations in progress or not locked yet
        <none>

    unlock() == true;
        <- unlockedCallback(); or <firmware restart>

    assumeUnlocked();
        <none>

*/

class IGlobalLockManager {
public:
    using ResultCallback = std::function< void( const std::string& error ) >;
    using ServiceCallback = std::function< void( const std::string& service ) >;
    using Callback = std::function< void() >;

    virtual bool start( ResultCallback lockedCallback,
                        Callback unlockedCallback,
                        ServiceCallback serviceRestartedWhileLockedCallback ) = 0;

    // NOTE: upon failed lock and corresponding callback is sent, auto unlock will be performed
    // if lock or unlock returns false, no corresponding failed callbacks will be sent.
    virtual bool lock() = 0;
    virtual bool unlock() = 0;
    virtual bool assumeUnlocked() = 0;

    virtual bool isLocked() const = 0;
    virtual CommonUtils::HubState getHubState() const = 0;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
