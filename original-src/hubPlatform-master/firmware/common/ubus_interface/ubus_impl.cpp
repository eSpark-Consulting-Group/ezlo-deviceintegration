#include "ubus_impl.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "tiny/tiny_consolecolors.h"
#include "json-c/json.h"

extern "C" {
#include <libubus.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
}

namespace HubPlatform {
namespace Ubus {

UBusImpl::~UBusImpl() {
    if( m_context != nullptr ) {
        ubus_free( m_context );
    }
}

void UBusImpl::runCallback( struct ubus_request* req, int type, struct blob_attr* msg ) {
    char* str;
    str = blobmsg_format_json_indent( msg, true, 0 );
    if( req->priv != nullptr ) {
        auto f = static_cast< std::function< void( std::string ) >* >( req->priv );
        ( *f )( std::string( str ) );
    }
    free( str );
}

namespace {
std::string toString( const char* value ) {
    if( value && *value )
        return value;
    return "";
}
}  // namespace

bool UBusImpl::connect( const char* uBusSocket ) {
    LOGGER_TRACE( cc::trace( "Connect to ubus: " ) + toString( uBusSocket ) );
    if( m_context != nullptr ) {
        ubus_free( m_context );
        m_context = nullptr;
        LOGGER_DEBUG( cc::debug( "Ubus was connected, freeing!" ) );
    }

    m_context = ubus_connect( uBusSocket );
    if( m_context == nullptr ) {
        LOGGER_ERROR( cc::error( "Failed to connect to uBus, name: " ) + cc::str( toString( uBusSocket ) ) +
                      ", error: " + cc::str( ubus_strerror( errno ) ) );
        return false;
    }
    return true;
}

bool UBusImpl::isConnected() const {
    return m_context != nullptr;
}

uint& UBusImpl::getObjId() {
    return m_objId;
}

int UBusImpl::checkConnection( const std::string& objName ) {
    LOGGER_TRACE( "Check UBus connection to " + objName );
    int uBusStatus = ubus_lookup_id( m_context, objName.c_str(), &m_objId );
    if( uBusStatus != UBUS_STATUS_OK ) {
        LOGGER_ERROR( cc::error( "UBus connection check failed, name: " + objName + cc::str( ", id: " ) +
                                 cc::num10( m_objId ) + ", reason: " ) +
                      cc::attention( ubus_strerror( uBusStatus ) ) );
    }
    return uBusStatus;
}

ubus_context* UBusImpl::getContext() {
    return m_context;
}

bool UBusImpl::sendCommand( const std::string& objName,
                            const std::string& command,
                            const std::string& msgKey,
                            const std::string& msgValue,
                            const replyCallback& replyFunction ) {
    if( m_context == nullptr ) {
        return false;
    }

    auto bufferBlob = std::make_unique< blob_buf >();

    int uBusInvokeRes = 0;

    int uBusStatus = checkConnection( objName );
    if( uBusStatus != UBUS_STATUS_OK ) {
        // TODO: EFWR-4524: No recovery action here
        return false;
    }

    blob_buf_init( bufferBlob.get(), 0 );

    blobmsg_add_string( bufferBlob.get(), msgKey.c_str(), msgValue.c_str() );

    ubus_data_handler_t* gs;
    if( replyFunction != nullptr ) {
        uBusInvokeRes = ubus_invoke( m_context,
                                     m_objId,
                                     command.c_str(),
                                     bufferBlob->head,
                                     runCallback,
                                     (void*) &replyFunction,
                                     m_replyWaitTimeOut * 1000 );
    }
    else {
        uBusInvokeRes = ubus_invoke(
            m_context, m_objId, command.c_str(), bufferBlob->head, nullptr, nullptr, m_replyWaitTimeOut * 1000 );
    }

    blob_buf_free( bufferBlob.get() );

    if( uBusInvokeRes != UBUS_STATUS_OK ) {
        Log::error( cc::error( "ubus_invoke() failed: " ) + std::string( ubus_strerror( uBusInvokeRes ) ) );
        return false;
    }

    return true;
}

bool UBusImpl::sendCommand( const std::string& objName,
                            const std::string& command,
                            const std::string& msgKey,
                            const std::string& msgValue,
                            std::string& commandReply ) {
    bool ret = false;

    ret = sendCommand(
        objName, command, msgKey, msgValue, [&commandReply]( const std::string& val ) { commandReply = val; } );

    return ret;
}

bool UBusImpl::addUBusObject( ubus_object& uBusObject ) {
    int ret = 0;

    if( m_context == nullptr ) {
        Log::error( cc::error( "First connect to ubus with connect() method!" ) );
        return false;
    }

    uloop_init();
    ubus_add_uloop( m_context );

    ret = ubus_add_object( m_context, &uBusObject );
    if( ret != 0 ) {
        Log::error( cc::error( "Failed to add object: " ) + std::string( ubus_strerror( ret ) ) );
        return false;
    }

    uloop_run();
    uloop_done();

    Log::trace( cc::trace( "Json-c version: " + std::string( json_c_version() ) ) );

    return true;
}

void UBusImpl::breakUloop() {
    LOGGER_INFO( "Send SIGTERM signal to uloop" );
    uloop_handle_sigint( SIGTERM );
}
}  // namespace Ubus
}  // namespace HubPlatform
