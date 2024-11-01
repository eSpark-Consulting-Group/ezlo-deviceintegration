#pragma once

#include <json_utils/json_fwd.h>
#include <string>
#include <vector>
#include <memory>

namespace HubPlatform {
namespace LuaNetwork {

class EventsEmitter {
public:
    struct ScanningResult {
        std::string ipAddress;
        std::string macAddress;
        std::string hostName;
        std::vector< int > ports;
        unsigned tries;
        unsigned errors;
    };
    using ScanningOptionResult = std::unique_ptr< ScanningResult >;

    virtual ~EventsEmitter() = default;

    virtual void emitConnectionEstablishedEvent( const std::string& eventHandler, int connectionHandle ) = 0;
    virtual void emitFailedToConnectEvent( const std::string& eventHandler, int connectionHandle ) = 0;
    virtual void emitIncomingDataEvent( const std::string& eventHandler, int connectionHandle ) = 0;
    virtual void emitConnectionClosedEvent( const std::string& eventHandler, int connectionHandle ) = 0;
    virtual void emitScanningResult( const std::string& eventHandler,
                                     int scanId,
                                     const std::string& status,
                                     ScanningOptionResult&& optionResult ) = 0;
    virtual void emitDhcpEvent( const std::string& eventHandler,
                                const std::string& eventType,
                                const Json& eventData ) = 0;
    virtual void emitReceivedFromEvent( std::string eventHandler,
                                        int connectionHandle,
                                        std::string userData,
                                        std::string data,
                                        std::string address,
                                        std::string port ) = 0;
    virtual void emitReceivedEvent( std::string eventHandler,
                                    int connectionHandle,
                                    std::string userData,
                                    std::string data ) = 0;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
