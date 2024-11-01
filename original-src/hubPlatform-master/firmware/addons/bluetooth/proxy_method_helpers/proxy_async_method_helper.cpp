#include "bluetooth/proxy_method_helpers/proxy_async_method_helper.h"

#include "bluetooth/log.h"
#include "bluetooth/exceptions/custom_setup_not_implemented.h"

namespace HubPlatform {

ProxyAsyncMethodHelper::ProxyAsyncMethodHelper( Proxy& proxy, std::string method, bool useCustomSetup )
: BaseProxyMethodHelper( proxy, std::move( method ) )
, m_useCustomSetup( useCustomSetup ) {}

void ProxyAsyncMethodHelper::callProxyMethod() {
    GDBusSetupFunction setupFunc = m_useCustomSetup ? static_cast< GDBusSetupFunction >( setupCallback ) : nullptr;

    if( callProxyMethod( this, setupFunc, nullptr ) == TRUE ) {
        LOG_DEBUG( "'%s' called", getMethod().c_str() );
    }
    else {
        LOG_WARN( "Can't call '%s'", getMethod().c_str() );
    }
}

void ProxyAsyncMethodHelper::setupCallback( DBusMessageIter* iter ) {
    setup( iter );
}

void ProxyAsyncMethodHelper::setupCallback( DBusMessageIter* iter, void* userData ) {
    static_cast< ProxyAsyncMethodHelper* >( userData )->setupCallback( iter );
}

void ProxyAsyncMethodHelper::setup( DBusMessageIter* /*iter*/ ) {
    throw CustomSetupNotImplemented();
}

}  // namespace HubPlatform
