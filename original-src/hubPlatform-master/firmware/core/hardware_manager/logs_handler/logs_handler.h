#pragma once

#include <memory>

class Led;

namespace HubPlatform {

namespace Services {
class Service;
}

namespace HardwareManager {

class HubStateIndicationManager;

class LogsHandler {
public:
    LogsHandler( Services::Service& service, HubStateIndicationManager& indicationManager, Led& powerLed );
    ~LogsHandler();
    bool saveLogs();

private:
    bool saveToCloud();
    bool saveToUsbStick();
    bool saveToStorage();

private:
    Services::Service& m_hostService;
    HubStateIndicationManager& m_indicationManager;
    Led& m_powerLed;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
