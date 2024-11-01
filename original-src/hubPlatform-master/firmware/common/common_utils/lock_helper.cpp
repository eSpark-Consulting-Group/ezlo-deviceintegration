#include "common_utils/lock_helper.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace CommonUtils {

class LockHelper::Transaction {};

LockHelper& LockHelper::global() {
    static LockHelper helper;
    return helper;
}

LockHelper::LockHelper()
: m_locked( false ) {}

bool LockHelper::tryLock( const JsonObject& json ) {
    auto reason = json_get_str( json, "reason" );
    LOGGER_INFO_RAW( cc::info( "Locking for a " ) + cc::note( reason ) );

    std::lock_guard< std::mutex > lock( m_mutex );
    if( m_transactions.empty() && m_weakTransaction.expired() ) {
        m_locked = true;
        m_error = "";
        return true;
    }
    m_error = "Transactions running";
    LOGGER_ERROR_RAW( cc::error( "Failed to lock: " ) + cc::note( m_error ) );
    return false;
}

void LockHelper::lock( const JsonObject& json ) {
    auto reason = json_get_str( json, "reason" );
    LOGGER_INFO_RAW( cc::info( "Locking for a " ) + cc::note( reason ) );

    std::lock_guard< std::mutex > lock( m_mutex );
    m_locked = true;
    m_error = "";
}

void LockHelper::unlock( const JsonObject& json ) {
    auto reason = json_get_str( json, "reason" );
    LOGGER_INFO_RAW( cc::info( "Unlocking from a " ) + cc::note( reason ) );

    std::unique_lock< std::mutex > lock( m_mutex );
    m_locked = false;
    m_error = "";

    for( auto& postponed : m_postponedFunctions ) {
        lock.unlock();
        postponed();
        lock.lock();
    }
    m_postponedFunctions.clear();
}

bool LockHelper::isLocked() const {
    std::lock_guard< std::mutex > lock( m_mutex );
    return m_locked;
}

bool LockHelper::isTransactionInProgress() const {
    std::lock_guard< std::mutex > lock( m_mutex );
    return !m_transactions.empty() || !m_weakTransaction.expired();
}

std::shared_ptr< LockHelper::Transaction > LockHelper::tryCreateTransaction() {
    std::lock_guard< std::mutex > lock( m_mutex );
    if( m_locked ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to start transaction: service locked" ) );
        return nullptr;
    }
    auto transaction = m_weakTransaction.lock();
    if( !transaction ) {
        transaction = std::make_shared< LockHelper::Transaction >();
        m_weakTransaction = transaction;
    }
    return transaction;
}

std::shared_ptr< LockHelper::Transaction > LockHelper::createTransaction() {
    std::lock_guard< std::mutex > lock( m_mutex );
    auto transaction = m_weakTransaction.lock();
    if( !transaction ) {
        transaction = std::make_shared< LockHelper::Transaction >();
        m_weakTransaction = transaction;
    }
    return transaction;
}

bool LockHelper::tryStartTransaction( int id ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    if( m_locked ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to start transaction: service locked" ) );
        return false;
    }
    m_transactions.insert( id );
    return true;
}

void LockHelper::onTransactionRunning( int id ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_transactions.insert( id );
}

void LockHelper::onTransactionStopped( int id ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_transactions.erase( id );
}

void LockHelper::postponeIfLocked( std::function< void() > function ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( !m_locked ) {
        lock.unlock();
        function();
        return;
    }

    m_postponedFunctions.push_back( std::move( function ) );
}

Json LockHelper::getResponse() const {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( !m_error.empty() ) {
        return createFailedResponse( m_error );
    }

    return createSuccessResponse();
}

Json LockHelper::createFailedResponse( const std::string& reason ) {
    auto response = JSON_EMPTY_OBJ;
    response[ "success" ] = false;
    setError( response, Errors::GLOBAL_LOCK_FAILED, reason );
    return response;
}

Json LockHelper::createSuccessResponse() {
    auto response = JSON_EMPTY_OBJ;
    response[ "success" ] = true;
    return response;
}

}  // namespace CommonUtils
}  // namespace HubPlatform
