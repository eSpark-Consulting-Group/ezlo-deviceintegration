
#pragma once
#include "log_writer_interface.h"
#include "ubus_interface/ubus_interface.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace LogReader {

using UbusPtr = std::unique_ptr< Ubus::UBusInterface >;
class UbusLogWriter final : public LogWriterInterface {
public:
    UbusLogWriter( const UbusLogWriter& ) = delete;
    UbusLogWriter( UbusLogWriter&& ) = delete;
    UbusLogWriter& operator=( const UbusLogWriter& ) = delete;
    UbusLogWriter& operator=( UbusLogWriter&& ) = delete;

    explicit UbusLogWriter( UbusPtr ubusPtr,
                            const std::string& serviceName,
                            const std::string& ubusServer,
                            const std::string& targetMethod );
    void sendLog( const std::string& data ) override;

private:
    bool checkUbusConnection();

private:
    const std::string& m_serviceName;
    const std::string& m_ubusServerName;
    const std::string& m_ubusMethod;
    UbusPtr m_uBusObject = nullptr;
};

}  // namespace LogReader
}  // namespace HubPlatform
