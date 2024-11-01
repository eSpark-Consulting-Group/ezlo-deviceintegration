#pragma once

#include "config_reader/config_reader.h"

#include <sstream>
#include <algorithm>

namespace HubPlatform {
namespace UT {

class ConfigReaderFactory {
protected:
    using JsonProviderFunction = std::function< std::string( const std::string& ) >;

protected:
    static ConfigReader createConfigReader( JsonProviderFunction jsonConfigurationProvider ) {
        return ConfigReader(
            [jsonConfigurationProvider]( const std::string& source ) {
                const auto configuration = jsonConfigurationProvider( source );
                return ConfigReader::StreamUPtr( !configuration.empty() ? new std::istringstream( configuration ) :
                                                                          nullptr );
            },
            {} );
    }
    static ConfigReader createConfigReaderWithSpecialSources(
        JsonProviderFunction mainJsonConfigurationProvider,
        std::map< std::string, JsonProviderFunction >&& specialConfigurationProviders ) {
        std::map< std::string, ConfigReader::SourceReader > specialSourceReaders;
        for( auto& provider : specialConfigurationProviders ) {
            const std::string& source = provider.first;
            auto specialSourceReaderCreator = [provider]( const std::string& source ) {
                const auto configuration = provider.second( source );
                return ConfigReader::StreamUPtr( !configuration.empty() ? new std::istringstream( configuration ) :
                                                                          nullptr );
            };

            specialSourceReaders.emplace( source, specialSourceReaderCreator );
        }

        return ConfigReader(
            [mainJsonConfigurationProvider]( const std::string& source ) {
                const auto configuration = mainJsonConfigurationProvider( source );
                return ConfigReader::StreamUPtr( !configuration.empty() ? new std::istringstream( configuration ) :
                                                                          nullptr );
            },
            specialSourceReaders );
    }
};

}  // namespace UT
}  // namespace HubPlatform
