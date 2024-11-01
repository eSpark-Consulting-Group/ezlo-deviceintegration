#include "bluetooth/proxy_method_helpers/proxy_sync_method_helper.h"

#include "bluetooth/log.h"
#include "bluetooth/exceptions/custom_setup_not_implemented.h"

#include <gdbus.h>

namespace HubPlatform {

ProxySyncMethodHelper::ProxySyncMethodHelper( Proxy& proxy,
                                              std::string method,
                                              bool useCustomSetup,
                                              std::chrono::seconds timeout )
: BaseProxyMethodHelper( proxy, std::move( method ) )
, m_timeout( timeout )
, m_useCustomSetup( useCustomSetup ) {}

void ProxySyncMethodHelper::callProxyMethod() {
    LOG_DEBUG( "Attempting to call '%s'", getMethod().c_str() );

    GDBusSetupFunction setupFunc = m_useCustomSetup ? static_cast< GDBusSetupFunction >( setupCallback ) : nullptr;

    // GDBus callback timeout is too big so we need to reset helper context
    auto helperContextHolder = new ProxySyncMethodHelper*( this );
    if( callProxyMethod( helperContextHolder, setupFunc, ProxySyncMethodHelper::returnCallback ) ) {
        const auto resultSignalFuture = m_resultSignal.get_future();
        const auto status = resultSignalFuture.wait_for( m_timeout );
        switch( status ) {
        case std::future_status::deferred:
        case std::future_status::timeout:
            *helperContextHolder = nullptr;  // reset 'this' context
            LOG_ERROR( "'%s' call timeout expired", getMethod().c_str() );
            break;

        case std::future_status::ready:
            break;
        }
    }
    else {
        delete helperContextHolder;
    }
}

void ProxySyncMethodHelper::setupCallback( DBusMessageIter* iter ) {
    setup( iter );
}

void ProxySyncMethodHelper::setupCallback( DBusMessageIter* iter, void* userData ) {
    auto helperContextHolder = static_cast< ProxySyncMethodHelper** >( userData );
    if( auto context = *helperContextHolder ) {
        context->setupCallback( iter );
    }
}

void ProxySyncMethodHelper::setup( DBusMessageIter* /*iter*/ ) {
    throw CustomSetupNotImplemented();
}

void ProxySyncMethodHelper::returnCallback( DBusMessage* message ) {
    DBusError error;
    dbus_error_init( &error );

    if( dbus_set_error_from_message( &error, message ) == FALSE ) {
        parseResult( message );
        LOG_DEBUG( "'%s' called successfully", getMethod().c_str() );
    }
    else {
        LOG_ERROR( "'%s' called with error '%s': %s", getMethod().c_str(), error.name, error.message );
        dbus_error_free( &error );
    }

    m_resultSignal.set_value();
}

void ProxySyncMethodHelper::returnCallback( DBusMessage* message, void* userData ) {
    auto helperContextHolder = static_cast< ProxySyncMethodHelper** >( userData );
    if( auto context = *helperContextHolder ) {
        context->returnCallback( message );
    }
    delete helperContextHolder;
}

void ProxySyncMethodHelper::parseResult( DBusMessage* /*message*/ ) {}

}  // namespace HubPlatform
