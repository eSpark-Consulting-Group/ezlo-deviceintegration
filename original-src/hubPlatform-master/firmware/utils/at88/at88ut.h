#pragma once
#include <string>

std::string getHubType();
std::string getHubSerial( const std::string& hubType );
std::string getHubMAC( const std::string& hubType );
std::string getFirmwareVersion();
std::string getHubHwKey( const std::string& hubType );  // probably valid only for vera hubs
