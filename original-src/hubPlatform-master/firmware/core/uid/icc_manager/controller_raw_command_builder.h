#pragma once

#include "json_utils/json_fwd.h"

#include <chrono>

namespace HubPlatform {
namespace InterControllerCommunication {

// created according to https://confluence.mios.com/display/EPD/Controller+-+Raw+Command

class ControllerRawCommandBuilder {
public:
    ControllerRawCommandBuilder( Json command, const std::string& controllerUuid );

    void setQueued( bool value );
    void setInstant( bool value );

    void setExpiryTimestamp( std::chrono::seconds value );
    void setInstantTimeout( std::chrono::seconds value );

    void setHistoryId( const std::string& id );

    Json build( const std::string& messageId );

private:
    Json m_command;
};

}  // namespace InterControllerCommunication
}  // namespace HubPlatform
