#pragma once

#include "json_utils/json_fwd.h"

#include <functional>
#include <memory>
#include <set>
#include <mutex>

namespace HubPlatform {
namespace CommonUtils {

class LockHelper final {
public:
    class Transaction;

    static LockHelper& global();

    LockHelper();

    // Lock/Unlock
    bool tryLock( const JsonObject& request );
    void lock( const JsonObject& request );
    void unlock( const JsonObject& request );
    bool isLocked() const;

    // Transactions
    bool isTransactionInProgress() const;

    // NOTE: Need to create transaction only if it takes more than one task
    // in main task loop or is not bound to main taskloop in the first place
    std::shared_ptr< Transaction > tryCreateTransaction();
    std::shared_ptr< Transaction > createTransaction();

    bool tryStartTransaction( int id );
    void onTransactionRunning( int id );
    void onTransactionStopped( int id );

    // Postpone
    void postponeIfLocked( std::function< void() > function );

    // Returnes failed response if previous tryLock/lock/unlock failed
    Json getResponse() const;

    // Can be used if there is other logic that can fail outside this helper
    static Json createFailedResponse( const std::string& reason );
    static Json createSuccessResponse();

private:
    mutable std::mutex m_mutex;
    bool m_locked;
    std::string m_error;
    std::weak_ptr< Transaction > m_weakTransaction;
    std::set< int > m_transactions;

    std::vector< std::function< void() > > m_postponedFunctions;
};

}  // namespace CommonUtils
}  // namespace HubPlatform
