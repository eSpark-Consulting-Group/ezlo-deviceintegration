#pragma once

#include <string>

namespace HubInfoHelper {

// TODO: move to Environment class

void setExplicitSerial( const std::string& serial );
void setExplicitHubType( const std::string& hubType );

const std::string& getSerial();
const std::string& getControllerUUID();
const std::string& getPkDevice();
const std::string& getHubType();
const std::string& getHardwareRevision();
const std::string& getMacAddress();
const std::string& getWifiMacAddress();
const std::string& getHardwareKey();
const std::string& getCpuSid();

}  // namespace HubInfoHelper
