#include "ubus_log_writer.h"
#include "json_utils/json_wrapper.h"

#if !defined( __HUB_TYPE_G150__ ) && !defined( __HUB_TYPE_VHUB__ )
extern "C" {
#include <libubus.h>
}
#include "ubus_interface/ubus_interface.h"
#else
#define UBUS_STATUS_OK 1
#endif

namespace HubPlatform {
namespace LogReader {
UbusLogWriter::UbusLogWriter( UbusPtr ubusPtr,
                              const std::string& serviceName,
                              const std::string& ubusServerName,
                              const std::string& ubusMethod )

: m_uBusObject( std::move( ubusPtr ) )
, m_serviceName( serviceName )
, m_ubusServerName( ubusServerName )
, m_ubusMethod( ubusMethod ) {
    if( m_uBusObject )
        m_uBusObject->connect( nullptr );
}

void UbusLogWriter::sendLog( const std::string& logData ) {
    if( m_uBusObject ) {
        if( !checkUbusConnection() ) {
            m_uBusObject->connect( nullptr );
            if( !checkUbusConnection() ) {
                return;
            }
        }

        auto logJson = JSON( m_serviceName << logData );
        m_uBusObject->sendCommand( m_ubusServerName, m_ubusMethod, "msg", logJson.dump(), nullptr );
    }
}

bool UbusLogWriter::checkUbusConnection() {
    return m_uBusObject->isConnected() && m_uBusObject->checkConnection( m_ubusServerName ) == UBUS_STATUS_OK;
}

}  // namespace LogReader
}  // namespace HubPlatform
