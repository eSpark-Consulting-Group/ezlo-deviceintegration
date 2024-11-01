#pragma once
#include "base_event.h"

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class ProtectSosButtonEvent : public BaseEvent {
public:
    ProtectSosButtonEvent( const std::string& eventName, std::string deviceId, std::string service );

    Json toJson() const override;

protected:
    std::string m_deviceId;
    std::string m_service;
};

class ProtectSosButtonAdded : public ProtectSosButtonEvent {
public:
    ProtectSosButtonAdded( std::string deviceId, std::string service );
};

class ProtectSosButtonUpdated : public ProtectSosButtonEvent {
public:
    ProtectSosButtonUpdated( std::string deviceId, std::string service );
};

class ProtectSosButtonRemoved : public ProtectSosButtonEvent {
public:
    explicit ProtectSosButtonRemoved( std::string deviceId );

    Json toJson() const override;
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
