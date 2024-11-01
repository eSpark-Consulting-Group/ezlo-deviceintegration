#include "http/event_emitter_base.h"
#include "http/resources.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"
#include <string>

namespace HubPlatform {
namespace CommonHttp {

bool isTrace() {
    return Log::getLogLevel() >= Log::TRACE;
}

void HttpEventEmitterBase::verbose( bool value ) {
    m_verbose = value;
}

void HttpEventEmitterBase::dataSendEvent( const std::string& eventHandler, EventPtr eventPtr ) {
    if( !eventPtr.get() )
        return;
    emit( Resources::HttpEvents::HttpDataSend, eventHandler, eventPtr );
}

void HttpEventEmitterBase::dataReceived( const std::string& eventHandler, EventPtr eventPtr ) {
    if( !eventPtr.get() )
        return;
    emit( Resources::HttpEvents::HttpDataReceived, eventHandler, eventPtr );
}

void HttpEventEmitterBase::connectionClosed( const std::string& eventHandler, EventPtr eventPtr ) {
    if( !eventPtr.get() )
        return;
    LOGGER_TRACE_RAW( cc::trace( "HttpEmitter.connectionClosedEvent: " ) + cc::j( eventPtr->toJson() ) );
    emit( Resources::HttpEvents::HttpConnectionClosed, eventHandler, eventPtr );
}

void HttpEventEmitterBase::progressEvent( const std::string& eventHandler, EventPtr eventPtr ) {
    emit( Resources::HttpEvents::HttpProgressEvent, eventHandler, eventPtr );
}

}  // namespace CommonHttp
}  // namespace HubPlatform
