#include "database_client/dbqueryparser.h"
#include "hub_settings/hub_settings_manager.h"
#include "hub_settings/hub_setting.h"
#include "hub_settings/resources.h"

#include "config_reader/config_keys.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {

namespace {

Json getSettingJson( const ConfigReader::Path& path ) {
    try {
        auto name = ConfigReader::getString( path / "name" );
        auto valueType = ConfigReader::getString( path / "valueType" );
        if( valueType == "token" ) {
            auto valueEnum = ConfigReader::getStringArray( path / "enum" );
            auto value = ConfigReader::getString( path / "value" );
            return JSON( "_id" << name << "valueType" << valueType << "enum" << valueEnum << "value" << value );
        }
        else if( valueType == "bool" ) {
            auto value = ConfigReader::getBool( path / "value" );
            return JSON( "_id" << name << "valueType" << valueType << "value" << value );
        }
        else if( valueType == "string" ) {
            auto value = ConfigReader::getString( path / "value" );
            return JSON( "_id" << name << "valueType" << valueType << "value" << value );
        }
        else if( valueType == "int" ) {
            auto value = ConfigReader::getInt( path / "value" );
            return JSON( "_id" << name << "valueType" << valueType << "value" << value );
        }

        Log::error( cc::error( "Setting " ) + cc::note( name ) + cc::error( " has invalid valueType: " ) +
                    cc::note( valueType ) );
    }
    catch( const std::exception& error ) {
        Log::error( cc::error( "Error reading setting: " ) + cc::note( error.what() ) );
    }

    return {};
}

}  // namespace

namespace Hub {

SettingsManager::SettingsManager( IDbClient& dbClient, const ConfigReader& configReader )
: m_dbClient( dbClient )
, m_configReader( configReader ) {
    using namespace Resources;

    try {
        auto dbSettings = m_dbClient.findAll( DBNames::SettingsDB, db::Query(), nullptr, 0, 0, true );
        std::map< std::string, Hub::Setting > storedSettings;
        for( auto& setting : dbSettings ) {
            std::string name = setting.at( "_id" ).get< std::string >();
            storedSettings.emplace( name, setting );
        }

        // FIXME: Reads from house_modes.cfg instead of settings.cfg in cored tests, throws an exception:
        // Path: settings/settings -> house_modes.cfg/settings
        ConfigReader::Path settingsPath( m_configReader, ConfigKeys::Settings );
        auto settingsCount = ConfigReader::getArraySize( settingsPath );
        for( size_t i = 0; i < settingsCount; ++i ) {
            auto settingJson = getSettingJson( settingsPath / i );
            auto setting = Hub::Setting( settingJson );
            if( !setting.isValid() ) {
                Log::error( cc::error( "Ignoring malformed setting: " + settingJson.dump() ) );
                continue;
            }
            auto storedSettingIt = storedSettings.find( setting.getName() );
            if( storedSettingIt == storedSettings.cend() ) {
                m_dbClient.insert( DBNames::SettingsDB, settingJson );
                storedSettings.emplace( setting.getName(), setting );
                continue;
            }

            auto& storedSetting = storedSettingIt->second;
            if( storedSetting.getValueType() != setting.getValueType() ) {
                Log::warn( cc::warn( "Value type has changed for setting: " + storedSetting.getName() ) );
                m_dbClient.update( DBNames::SettingsDB, JSON_EMPTY_OBJ, settingJson, true );
                continue;
            }

            if( setting.getValueType() == Setting::ValueType::Token ) {
                if( storedSetting.getPossibleTokenValues() != setting.getPossibleTokenValues() ) {
                    Log::warn( cc::warn( "Token range mismatch for setting: " + storedSetting.getName() ) );
                    if( !setting.setTokenValue( storedSetting.getTokenValue() ) )
                        Log::warn( cc::warn( "Stored token value is not in valid range. Restore to default value" ) );
                    settingJson[ "value" ] = setting.getTokenValue();
                    m_dbClient.update( DBNames::SettingsDB, JSON_EMPTY_OBJ, settingJson, true );
                }
            }
        }
    }
    catch( const std::exception& error ) {
        Log::warn( cc::warn( "Cannot read default settings: " ) + cc::error( error.what() ) );
    }
    catch( ... ) {
        Log::warn( cc::warn( "Cannot read default settings: Unknown exception" ) );
    }
}

bool SettingsManager::setSettingValue( const std::string& name, const Json& value ) {
    using namespace Resources;
    auto prev = m_dbClient.findOne( DBNames::SettingsDB, JSON( db::ID_FIELD_NAME << name ), nullptr );

    if( prev.empty() ) {
        Log::warn( cc::warn( "Attempt to set value for nonexistent setting name: " + name ) );
        return false;
    }

    if( prev[ "value" ] == value ) {
        Log::warn( cc::warn( "New setting value is equal to previous one, skip: old = " ) + cc::j( prev[ "value" ] ) +
                   cc::warn( ", new: " ) + cc::j( value ) );
        return false;
    }

    auto setting = Hub::Setting( prev );
    if( !setting.setValue( value ) ) {
        Log::warn( cc::warn( "Attempt to set invalid setting value: " + value.dump() ) );
        return false;
    }

    m_dbClient.update(
        DBNames::SettingsDB, JSON( db::ID_FIELD_NAME << name ), JSON( "$set" << JSON( "value" << value ) ) );
    LOGGER_TRACE( cc::trace( "Setting successfully updated" ) );
    return true;
}

}  // namespace Hub
}  // namespace HubPlatform
