#include "log_settings.h"

#include <iostream>
#include <map>
#include <string>

#include "hub_settings/hub_settings_reader.h"
#include "database_client/lmdb_client.h"
#include "database_client/dbtools.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

#include "logcmd/log_exceptions.h"

namespace HubPlatform {
namespace LogCmd {

namespace {

namespace LogSettingName {
const char* const rotationInterval = "logs.rotation.interval";
const char* const rotationSizeLimit = "logs.rotation.size_limit";
const char* const fileSizeLimit = "logs.file.size_limit";
const char* const pendingLogFolderSizeLimit = "logs.pending.folder.size_limit";
}  // namespace LogSettingName

void readLogRotationPolicy( Hub::SettingsReader& reader, LogSettings& logSettings ) {
    using RotationPolicy = LogSettings::RotationPolicy;
    const static std::map< std::string, RotationPolicy > policyMap = {
        { "off", RotationPolicy::Off },
        { "each_rotation", RotationPolicy::EachRotation },
        { "store_locally", RotationPolicy::StoreLocally }
    };

    auto setting = reader.findSetting( "logs.send_mode" );
    if( !setting ) {
        LOGGER_WARN_RAW( cc::warn( "Unable to read rotation policy. Use default policy" ) );
        return;
    }

    std::string policy = setting->getTokenValue();
    auto it = policyMap.find( policy );
    if( it == policyMap.cend() ) {
        LOGGER_WARN_RAW( cc::warn( "Unknown rotation policy ( " ) + cc::note( policy ) +
                         cc::warn( " ). Use default policy" ) );
        return;
    }

    LOGGER_TRACE_RAW( cc::trace( "Use rotation policy: " ) + cc::note( policy ) );
    logSettings.rotationPolicy = it->second;
}

template < typename T >
void readLogNumericValue( Hub::SettingsReader& reader, const std::string& name, T& value ) {
    auto setting = reader.findSetting( name );
    if( !setting ) {
        LOGGER_WARN_RAW( cc::warn( "Unable to read setting: " ) + cc::note( name ) +
                         cc::warn( " : Use default value" ) );
        return;
    }

    int settingValue = setting->getIntValue();
    if( settingValue <= 0 ) {
        LOGGER_WARN_RAW( cc::warn( "Invalid " ) + cc::note( name ) + cc::warn( " value ( " ) +
                         cc::note( std::to_string( settingValue ) ) + cc::warn( " ). Use default value" ) );
        return;
    }

    value = settingValue;
    LOGGER_TRACE_RAW( cc::trace( "Use " ) + cc::note( name ) + cc::trace( " value ( " ) +
                      cc::note( std::to_string( value ) ) + cc::trace( " )" ) );
}

}  // namespace

void loadLogSettings( LogSettings& logSettings ) {
    try {
        std::string dbPath = getStorageFilePath();
        if( dbPath.empty() )
            throw LogDatabaseError( "Database path is empty" );

        LMDBClient dbClient( dbPath, getTemporaryStorageFilePath() );
        Hub::SettingsReader reader( dbClient );

        readLogRotationPolicy( reader, logSettings );
        readLogNumericValue( reader, LogSettingName::rotationInterval, logSettings.rotationInterval );

        size_t rotatedLogsSizeLimitMb = 0;
        readLogNumericValue( reader, LogSettingName::rotationSizeLimit, rotatedLogsSizeLimitMb );
        logSettings.setArchivesTotalSizeLimit( mbToBytes( rotatedLogsSizeLimitMb ) );

        size_t fileSizeLimitKb = 0;
        readLogNumericValue( reader, LogSettingName::fileSizeLimit, fileSizeLimitKb );
        logSettings.setSliceFileSizeLimit( kbToBytes( fileSizeLimitKb ) );

        size_t pendingLogsFolderSizeLimitMb = 0;
        readLogNumericValue( reader, LogSettingName::pendingLogFolderSizeLimit, pendingLogsFolderSizeLimitMb );
        logSettings.setSlicePendingSizeLimit( mbToBytes( pendingLogsFolderSizeLimitMb ) );
    }
    catch( std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Unable to process controller settings. Exception (" +
                                     std::string( typeid( e ).name() ) + "): " ) +
                          cc::note( e.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Unable to process controller settings. Unknown exception was caught" ) );
    }
}

void LogSettings::setArchivesTotalSizeLimit( size_t sizeLimit ) {
    if( sizeLimit > 0 )
        this->archivesTotalSizeLimit = sizeLimit;
}

void LogSettings::setSliceFileSizeLimit( size_t sizeLimit ) {
    if( sizeLimit > 0 )
        this->fileSizeLimit = sizeLimit;
}

void LogSettings::setSlicePendingSizeLimit( size_t sizeLimit ) {
    if( sizeLimit > 0 )
        this->pendingLogsSizeLimit = sizeLimit;
}

}  // namespace LogCmd
}  // namespace HubPlatform
