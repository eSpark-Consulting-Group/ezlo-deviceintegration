#include "cloud_command.h"
#include "database_client/dbid.h"
#include "resources.h"

namespace HubPlatform {

using namespace Resources;
using namespace Resources::Parameters;

CloudCommand::CloudCommand( int retries,
                            const Json& data,
                            SendCommandCallback sendCallback,
                            SuccesCallback successCallback,
                            NotifyCloudCallback failCallback,
                            const std::string& id )
: m_maxRetries( retries )
, m_data( data )
, m_sendCallback( std::move( sendCallback ) )
, m_successCallback( std::move( successCallback ) )
, m_failCallback( std::move( failCallback ) )
, m_id( id ) {
    if( m_id.empty() ) {
        m_id = db::DbId::genOID();
    }

    m_remainingRetries = m_maxRetries;
}

CloudCommand::CloudCommand( const Json& storedRecord,
                            SendCommandCallback sendCallback,
                            SuccesCallback successCallback,
                            NotifyCloudCallback failCallback )
: m_maxRetries( storedRecord[ RetriesNumber ] )
, m_data( storedRecord[ CommandData ] )
, m_sendCallback( std::move( sendCallback ) )
, m_successCallback( std::move( successCallback ) )
, m_failCallback( std::move( failCallback ) )
, m_id( json_get_str( storedRecord, Id ) ) {
    m_remainingRetries = m_maxRetries;
}

void CloudCommand::onFailure() {
    if( m_failCallback ) {
        m_failCallback( data() );
    }
}

void CloudCommand::onSuccess() {
    if( m_successCallback ) {
        m_successCallback( data() );
    }
}

void CloudCommand::decrementRemainingRetries() {
    m_remainingRetries--;
}

void CloudCommand::execute() {
    if( m_sendCallback ) {
        m_sendCallback( m_data );
    }
}

ExecuteStatus CloudCommand::checkResult() {
    std::lock_guard< std::mutex > lock( m_lock );
    if( m_status == ReceiveStatus::Ok )
        return ExecuteStatus::AnswerOk;
    else if( m_status == ReceiveStatus::Error )
        return ExecuteStatus::AnswerError;
    else {
        return ExecuteStatus::Fail;
    }
}

void CloudCommand::receive( bool status ) {
    {
        std::lock_guard< std::mutex > lock( m_lock );
        if( status )
            m_status = ReceiveStatus::Ok;
        else
            m_status = ReceiveStatus::Error;
    }
}

void CloudCommand::fail() {
    std::lock_guard< std::mutex > lock( m_lock );
    m_isReceived = true;
    m_status = ReceiveStatus::Timeout;
}

int CloudCommand::getRemainingRetries() const {
    return m_remainingRetries;
}

Json CloudCommand::data() {
    return m_data;
}

std::string CloudCommand::id() {
    return m_id;
}

int CloudCommand::getRetryIndex() const {
    auto index = m_maxRetries - m_remainingRetries;
    return m_maxRetries - m_remainingRetries;
}

}  // namespace HubPlatform
