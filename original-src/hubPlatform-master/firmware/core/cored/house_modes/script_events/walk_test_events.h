#pragma once
#include "base_event.h"

#include <chrono>

namespace HubPlatform {
namespace CoreDHouseModes {
namespace ScriptEvents {

class WalkTestEvent : public BaseEvent {
public:
    WalkTestEvent( const std::string& eventName,
                   const std::string& operationId,
                   std::chrono::seconds timestamp,
                   std::chrono::seconds timeLeft );
    Json toJson() const override;

protected:
    std::string m_eventName = "";
    std::string m_operationId = "";
    std::chrono::seconds m_timestamp{ 0 };
    std::chrono::seconds m_timeLeft{ 0 };
};

class WalkTestEventStarted : public WalkTestEvent {
public:
    WalkTestEventStarted( const std::string& operationId,
                          std::chrono::seconds timestamp,
                          std::chrono::seconds timeLeft );
};

class WalkTestEventExtended : public WalkTestEvent {
public:
    WalkTestEventExtended( const std::string& operationId,
                           std::chrono::seconds timestamp,
                           std::chrono::seconds timeLeft );
};

class WalkTestEventFinished : public WalkTestEvent {
public:
    explicit WalkTestEventFinished( const std::string& operationId,
                                    std::chrono::seconds timestamp,
                                    std::chrono::seconds timeLeft );
};

class WalkTestEventTerminationProgress : public WalkTestEvent {
public:
    explicit WalkTestEventTerminationProgress( const std::string& operationId,
                                               std::chrono::seconds timestamp,
                                               std::chrono::seconds timeLeft );
};

}  // namespace ScriptEvents
}  // namespace CoreDHouseModes
}  // namespace HubPlatform
