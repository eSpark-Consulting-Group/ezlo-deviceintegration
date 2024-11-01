#pragma once

#include "networkd/gsm/nm_gsm.h"
#include "networkd/gsm/nm_at_executor.h"
#include "networkd/configuration/nm_config_file_manager.h"

#include "json_utils/json_utils.h"

#include <string>
#include <memory>

namespace HubPlatform {
namespace Network {
namespace GSM {
namespace Model {
extern const char* const QUECTEL_EC25;
}  // namespace Model

const JsonArray& gsmModemList();
const JsonObject getModemDescription( const std::string& model );
const JsonObject getModemDescriptionByUSB( const std::string& usbDescription );
std::string getModemModelByDevicePath( const std::string& devicePath );
std::unique_ptr< GSMModem > createGSMModem( const NetworkConfigFileManager::MobileConfiguration& configuration,
                                            const std::string& model,
                                            bool embeddedModem,
                                            const SerialSettings& serialSettings,
                                            const std::string& ownerName,
                                            bool linkedToPhysicalModem = true );
std::unique_ptr< GSMModem > createGSMModemNoLinkedToPhysical( const std::string& model, bool embeddedModem = true );
}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
