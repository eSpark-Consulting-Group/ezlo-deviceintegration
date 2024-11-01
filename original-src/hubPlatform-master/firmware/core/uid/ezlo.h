
#pragma once

#include "jsonrpc/message.h"

#include <string>

class BaseBus;

class Ezlo {
public:
    static void setDescription( const std::string& description );
    static std::string getDescription();
    static std::string getZwaveCalibration();
    static JsonArray getHubErrors();
    static void killSSH( uint time );
    static JsonObject getRuleInfo( const std::string ruleId );
    static bool runScript( const std::string name, JsonObject& o );
    static std::string serial();
    static std::string uuid();
    static int getTimeOffset();
    static JsonObject getMacs();
    static bool isConnected();
    static std::string getMedia();
    static const std::string& hubType();
    static const std::string& macAddress();
    static const std::string& getModel();
    static void reboot();

    static BaseBus& getBus();
    static BaseBus* getBusPtr();

    static void setBus( BaseBus* bus );

    // static void loginToNest(JsonObject &answer);
    // static std::string getNestPinCode();
    // static std::string getNestAccessToken();
    // TODO add nest set methods
private:
    static std::string getMac( const std::string& interface );
    static std::string evalHubType();

private:
    static BaseBus* m_bus;
};
