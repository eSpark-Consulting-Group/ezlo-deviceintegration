#include "networkd/gsm/nm_gsm_model.h"
#include "networkd/gsm/ec25/nm_ec25.h"

#include "system_utils/system.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Network {
namespace GSM {
namespace Model {
extern const char* const QUECTEL_EC25 = "Quectel EC25";
}  // namespace Model

const JsonArray& gsmModemList() {
    static const JsonArray modemList = {
        {
            { "model", Model::QUECTEL_EC25 },
            { "usbDescription", "2c7c:0125" },
        },
    };
    return modemList;
}

const JsonObject getModemDescription( const std::string& model ) {
    for( auto& modem : gsmModemList() )
        if( modem[ "model" ] == model )
            return modem;
    throw std::runtime_error( "Model \"" + model + "\" not found in a mobile modems list" );
}

const JsonObject getModemDescriptionByUSB( const std::string& usbDescription ) {
    for( auto& modem : gsmModemList() )
        if( modem[ "usbDescription" ] == usbDescription )
            return modem;
    throw std::runtime_error( "Model with usbDescription \"" + usbDescription + "\" not found in mobile modems list" );
}

std::string getModemModelByDevicePath( const std::string& devicePath ) {
    const auto usbDesc = System::getUSBDescription( devicePath );
    const auto usbDescStr = usbDesc.idVendor + ':' + usbDesc.idProduct;
    const auto modemDesc = getModemDescriptionByUSB( usbDescStr );
    return modemDesc[ "model" ];
}

std::unique_ptr< GSMModem > createGSMModem( const NetworkConfigFileManager::MobileConfiguration& configuration,
                                            const std::string& model,
                                            bool embeddedModem,
                                            const SerialSettings& serialSettings,
                                            const std::string& ownerName,
                                            bool linkedToPhysicalModem ) {
    if( model == Model::QUECTEL_EC25 )
        return std::make_unique< EC25 >(
            configuration, serialSettings, embeddedModem, linkedToPhysicalModem, ownerName );
    throw std::runtime_error( "Failed to create a mobile modem with unknown model: \"" + model + "\"" );
}

std::unique_ptr< GSMModem > createGSMModemNoLinkedToPhysical( const std::string& model, bool embeddedModem ) {
    return createGSMModem(
        NetworkConfigFileManager::MobileConfiguration{}, model, embeddedModem, SerialSettings{}, "", false );
}

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
