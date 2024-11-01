#include "cored/model/configuration/configuration_impl.h"
#include "cored/model/values/value_type.h"
#include "hub_settings/hub_setting.h"
#include "hub_settings/hub_settings_reader.h"
#include "service_interfaces/service.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

ConfigurationImpl::ConfigurationImpl( const IDbClient& dbClient ) noexcept {
    try {
        readSettings( dbClient );
    }
    catch( const std::exception& error ) {
        Log::error( cc::error( "could not load configuration: " ).append( error.what() ) );
    }
}

ConfigurationImpl::~ConfigurationImpl() = default;

std::string ConfigurationImpl::getScaleForValueType( const ValueType& valueType ) const {
    if( valueType == ValueType::Enum::Temperature ) {
        // Default scale was changed to fahrenheit
        if( m_temperatureScaleUPtr ) {
            auto loaded = m_temperatureScaleUPtr->getTokenValue();
            if( !loaded.empty() ) {
                return loaded;
            }
        }
        return ValueScales::Fahrenheit;
    }
    return std::string{};
}

void ConfigurationImpl::readSettings( const IDbClient& dbClient ) {
    Hub::SettingsReader reader( dbClient );
    m_temperatureScaleUPtr = reader.findSetting( "scale.temperature" );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
