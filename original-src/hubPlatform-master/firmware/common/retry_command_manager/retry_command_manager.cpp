#include "retry_command_manager.h"
#include "common_utils/cpp14.h"
#include "base_utils/timeutils.h"
#include <thread>
#include "resources.h"

namespace HubPlatform {

using namespace Resources;
using namespace Resources::Parameters;

const int MaxStorageRecordsNumber = 5000;

RetryCommandManager::RetryCommandManager( const std::string& commandClassId,
                                          std::chrono::seconds minIntervalSeconds,
                                          std::chrono::seconds maxIntervalSeconds,
                                          std::chrono::seconds persistentStoringIntervalSeconds,
                                          Services::Service& service,
                                          bool testMode )
: m_commandClassId( commandClassId )
, m_minIntervalSeconds( minIntervalSeconds )
, m_maxIntervalSeconds( maxIntervalSeconds )
, m_persistentStoringIntervalSeconds( persistentStoringIntervalSeconds )
, m_service( service )
, m_testMode( testMode ) {
    m_processTask = m_service.createPostponedTask( [this]( Services::PostponedTask& ) { processCommands(); } );
}

RetryCommandManager::~RetryCommandManager() {
    if( m_processTask->isActive() )
        m_processTask->stop();
}

std::vector< Json > RetryCommandManager::loadCommandsDataFromStorage() {
    return m_service.getDBClient().findAll(
        DBTableNames::CommandRetryStorage, JSON( CommandClassId << m_commandClassId ), nullptr );
}

void RetryCommandManager::processCommands() {
    std::unique_lock< std::mutex > lock( m_queueMutex );
    if( !m_commandQueue.empty() ) {
        m_command = std::move( m_commandQueue.front() );
        m_commandQueue.pop();
    }
    lock.unlock();
    if( !m_command ) {
        return;
    }
    m_command->execute();
}

void RetryCommandManager::checkResult() {
    auto status = m_command->checkResult();

    if( status == ExecuteStatus::AnswerOk ) {
        removeFromPersistentStorage( m_command->id() );
        m_command->onSuccess();
        m_command.reset();
    }
    else if( status == ExecuteStatus::AnswerError ) {
        removeFromPersistentStorage( m_command->id() );
        m_command->onFailure();
        m_command.reset();
    }
    else if( status == ExecuteStatus::Fail ) {
        m_command->decrementRemainingRetries();
        handleFailedState();
    }
}

void RetryCommandManager::handleFailedState() {
    auto id = m_command->id();
    LOGGER_INFO( cc::debug( "handleFailedState commandId: " ) + cc::debug( id ) );
    if( m_command->getRemainingRetries() > 0 ) {
        addToPersistentStorageIfNeeded( id );

        if( m_testMode ) {
            sendCommand( m_command );
        }
        else {
            increaseRetryInterval();
        }
    }
    else {
        LOGGER_INFO( cc::debug( "Command lifetime of the command is finished commandId: " ) + cc::debug( id ) );
        removeFromPersistentStorage( id );
        m_command->onFailure();
        m_command.reset();
    }
}

void RetryCommandManager::increaseRetryInterval() {
    std::lock_guard< std::mutex > lock( m_queueMutex );
    auto newInterval = calcRepeatInterval( m_command->getRetryIndex() );
    m_commandQueue.push( std::move( m_command ) );

    if( m_processTask->isActive() ) {
        m_processTask->stop();
    }

    m_processTask->start( newInterval, false );
}

void RetryCommandManager::removeFromPersistentStorage( const std::string& id ) {
    auto filter = JSON( Id << id );
    if( m_service.getDBClient().has( DBTableNames::CommandRetryStorage, filter ) ) {
        m_service.getDBClient().remove( DBTableNames::CommandRetryStorage, filter );
    }
}

void RetryCommandManager::addToPersistentStorageIfNeeded( const std::string& id ) {
    auto now = TimeUtils::getCurrentTime();
    if( now - m_latestStorageTimestamp >= m_persistentStoringIntervalSeconds.count() ||
        m_latestStorageTimestamp == 0 ) {
        m_latestStorageTimestamp = now;
        auto data = m_command->data();
        auto retriesNumber = m_command->getRemainingRetries();
        addOrUpdateDBRecord( id, data, retriesNumber );
    }
}

void RetryCommandManager::checkAndClearStorageIfNeeded() {
    if( m_service.getDBClient().size( DBTableNames::CommandRetryStorage ) > MaxStorageRecordsNumber ) {
        m_service.getDBClient().clear( DBTableNames::CommandRetryStorage );
    }
}

void RetryCommandManager::addOrUpdateDBRecord( const std::string& id, const Json& data, int retriesNumber ) {
    LOGGER_INFO( cc::debug( "Add command to DB commandId: " ) + cc::debug( id ) );
    auto record = m_service.getDBClient().findOne( DBTableNames::CommandRetryStorage, JSON( Id << id ), nullptr );
    if( record.empty() ) {
        record = JSON( Id << id << CommandData << data << CommandClassId << m_commandClassId << RetriesNumber
                          << retriesNumber );
        checkAndClearStorageIfNeeded();
        m_service.getDBClient().insert( DBTableNames::CommandRetryStorage, record );
    }
    else {
        record[ RetriesNumber ] = retriesNumber;
        m_service.getDBClient().update( DBTableNames::CommandRetryStorage, JSON( Id << id ), record );
    }
}

void RetryCommandManager::sendCommand( CommandPtr command ) {
    {
        LOGGER_INFO( cc::debug( "sendCommand commandId: " ) + cc::debug( command->id() ) );
        std::lock_guard< std::mutex > lock( m_queueMutex );
        m_commandQueue.push( std::move( command ) );
    }
    processCommands();
}

std::chrono::seconds RetryCommandManager::calcRepeatInterval( int retryIndex ) {
    std::vector< int > PredefinedIntervals = { 15, 45, 90, 150, 210 };
    int result;
    if( retryIndex > PredefinedIntervals.size() ) {
        result = PredefinedIntervals.back() + ( retryIndex - PredefinedIntervals.size() ) * 90;
    }
    else {
        result = PredefinedIntervals[ retryIndex - 1 ];
    }

    LOGGER_DEBUG( "New retry interval: " + cc::j( result ) );

    return std::chrono::seconds( result );
}

}  // namespace HubPlatform
