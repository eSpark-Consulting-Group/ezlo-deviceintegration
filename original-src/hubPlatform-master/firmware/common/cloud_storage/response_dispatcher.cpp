#include "response_dispatcher.h"
#include "http/http_events.h"
#include "resources.h"

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

void Dispatcher::registerRequest( const RequestId& requestId,
                                  const Json& metadata,
                                  OnRequestFinishedCallback callback,
                                  UploadContextPtr uploadContextPtr ) {
    InnerResponse innerResponse;

    HttpResponse& response = innerResponse.m_response;
    response.setId( requestId );
    response.setMetadata( metadata );
    innerResponse.m_onRequestFinished = std::move( callback );
    if( uploadContextPtr )
        innerResponse.m_uploadContext = std::move( uploadContextPtr );

    std::unique_lock< std::mutex > lock( m_requestsMutex );
    m_httpRequests.emplace( std::make_pair( requestId, innerResponse ) );
}

void Dispatcher::dispatchDataReceived( const CommonHttp::EventPtr& event ) {
    auto receivedPtr = dynamic_cast< CommonHttp::DataReceivedEvent* >( event.get() );
    if( !receivedPtr )
        throw std::invalid_argument( "Wrong http event type, http_data_received expected" );

    Locker lock( m_requestsMutex );
    auto foundIt = m_httpRequests.find( event->id() );
    if( foundIt == m_httpRequests.end() )
        return;

    HttpResponse& response = foundIt->second.m_response;
    response.appendData( reinterpret_cast< const std::int8_t* >( receivedPtr->data().data() ),
                         receivedPtr->data().size() );
    response.setHttpStatus( receivedPtr->code(), "" );
    if( receivedPtr->last() )
        response.setFinished();
    if( response.getMetadata().empty() )
        response.setMetadata( receivedPtr->userData() );
}

/** Search for a request object. If it has more data, extract new portion and return outside.
 * Outside code should take this portion and deliver it to m_http.sendData() for this request to which the event belongs
 * to. */
void Dispatcher::dispatchDataSend( const CommonHttp::EventPtr& event, CommonHttp::Http& http ) {
    auto dataSendPtr = dynamic_cast< CommonHttp::DataSendEvent* >( event.get() );
    if( !dataSendPtr )
        throw std::invalid_argument( "Wrong http event type, http_data_send expected" );

    Locker lock( m_requestsMutex );
    auto foundIt = m_httpRequests.find( event->id() );
    if( foundIt == m_httpRequests.end() )
        return;

    const auto& requestId = foundIt->first;
    auto& request = foundIt->second;
    UploadContextPtr uploadContext = request.m_uploadContext;
    if( uploadContext == nullptr || uploadContext->endOfStream() )
        return;

    // TODO: EFWR-3980: Make portionSize tunable: use 16K buffer in production, 4K buffer for tests
    auto data = uploadContext->readPortion( PORTION_SIZE_DEFAULT );
    bool isLast = data.size() < PORTION_SIZE_DEFAULT || uploadContext->endOfStream();

    http.sendData( Resources::CloudStorageRequestHandler, requestId, data, isLast );
}

void Dispatcher::dispatchConnectionClosed( const CommonHttp::EventPtr& event ) {
    auto closedPtr = dynamic_cast< CommonHttp::ConnectionClosedEvent* >( event.get() );
    if( !closedPtr ) {
        throw std::invalid_argument( "Wrong http event type, http_connection_closed expected" );
    }

    InnerResponse innerResponse;
    popHttpResponse( event->id(), innerResponse );

    innerResponse.m_response.setId( closedPtr->id() );
    innerResponse.m_response.setHttpStatus( closedPtr->httpCode(), closedPtr->httpCodeStatus() );
    innerResponse.m_response.setErrorReason( closedPtr->reasonCode(), closedPtr->reasonText() );
    innerResponse.m_response.setFinished();

    if( innerResponse.m_onRequestFinished != nullptr ) {
        // invalidates response/foundIt->second.m_response
        innerResponse.m_onRequestFinished( std::move( innerResponse.m_response ) );
    }
}

bool Dispatcher::popHttpResponse( const RequestId& requestId, InnerResponse& innerResponse ) {
    Locker lock( this->m_requestsMutex );
    auto foundIt = this->m_httpRequests.find( requestId );
    if( foundIt == m_httpRequests.end() )
        return false;

    innerResponse = std::move( foundIt->second );
    this->m_httpRequests.erase( foundIt );
    lock.unlock();

    return true;
}

bool Dispatcher::findRequest( const Dispatcher::RequestId& requestId, HttpResponse& httpResponse ) const {
    Locker lock( this->m_requestsMutex );
    auto foundIt = m_httpRequests.find( requestId );
    if( foundIt != m_httpRequests.end() ) {
        httpResponse = foundIt->second.m_response;
        return true;
    }
    return false;
}

void HttpResponse::setFinished() {
    m_finished = true;
}

bool HttpResponse::getFinished() const {
    return m_finished;
}

void HttpResponse::setId( std::string id ) {
    m_id = std::move( id );
}

const std::string& HttpResponse::getId() const {
    return m_id;
}

void HttpResponse::setMetadata( Json metadata ) {
    m_metadata = std::move( metadata );
}

const Json& HttpResponse::getMetadata() const {
    return m_metadata;
}

void HttpResponse::appendData( const std::int8_t* data, unsigned int size ) {
    if( data != nullptr && size > 0 )
        m_body.insert( m_body.end(), data, data + size );
}

void HttpResponse::setErrorReason( int code, const std::string& description ) {
    m_reasonCode = code;
    m_reasonText = description;
}

void HttpResponse::setHttpStatus( int httpStatus, const std::string& error ) {
    m_httpStatus = httpStatus;
    m_httpStatusText = error;
}

const HttpResponse::Body& HttpResponse::getBody() const {
    return m_body;
}

HttpResponse::Body& HttpResponse::getBody() {
    return m_body;
}

int HttpResponse::getHttpStatus() const {
    return m_httpStatus;
}

const std::string& HttpResponse::getHttpStatusText() const {
    return m_httpStatusText;
}

int HttpResponse::getErrorReasonCode() const {
    return m_reasonCode;
}

const std::string& HttpResponse::getErrorReasonText() const {
    return m_reasonText;
}

bool HttpResponse::hasError() const {
    return getErrorReasonCode() != 0 || hasHttpStatusError();
}

// @returns true if http status is not in 200-399
bool HttpResponse::hasHttpStatusError() const {
    return getHttpStatus() < 200 && getHttpStatus() >= 400;
}

// @returns true if http status is not in 200-299
bool HttpResponse::hasHttpStatusNot200() const {
    return getHttpStatus() < 200 && getHttpStatus() >= 300;
}

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform