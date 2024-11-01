#include "zigbee/interview_configurations/node_interview_names_extractors.h"

#include <regex>

namespace HubPlatform {
namespace Zigbee {

std::unique_ptr< NodeInterviewNamesExtractors > NodeInterviewNamesExtractors::create( const Json& configurations ) {
    auto manufacturerNamesExtarctorsIt = configurations.find( "manufacturer_name_extractors" );
    auto modelNamesExtarctorsIt = configurations.find( "model_name_extractors" );

    if( manufacturerNamesExtarctorsIt != configurations.end() || modelNamesExtarctorsIt != configurations.end() ) {
        return std::unique_ptr< NodeInterviewNamesExtractors >{ new NodeInterviewNamesExtractors(
            manufacturerNamesExtarctorsIt == configurations.end() ? JSON_EMPTY_ARR : *manufacturerNamesExtarctorsIt,
            modelNamesExtarctorsIt == configurations.end() ? JSON_EMPTY_ARR : *modelNamesExtarctorsIt ) };
    }

    return nullptr;
}

common::optional< std::string > NodeInterviewNamesExtractors::getExtractedManufacturerName(
    const std::string& manufacturerName ) const {
    return getExtractedName( manufacturerName, m_manufacturerNamesExtarctors );
}

common::optional< std::string > NodeInterviewNamesExtractors::getExtractedModelName(
    const std::string& modelName ) const {
    return getExtractedName( modelName, m_modelNamesExtarctors );
}

NodeInterviewNamesExtractors::NodeInterviewNamesExtractors( const Json& manufacturerNamesExtarctors,
                                                            const Json& modelNamesExtarctors )
: m_manufacturerNamesExtarctors( manufacturerNamesExtarctors )
, m_modelNamesExtarctors( modelNamesExtarctors ) {}

common::optional< std::string > NodeInterviewNamesExtractors::getExtractedName( const std::string& name,
                                                                                const Json& extractors ) {
    for( const auto& extractor : extractors ) {
        const auto& regexIt = extractor.find( "regex" );
        const auto& nameIt = extractor.find( "name" );
        if( regexIt != extractor.end() && nameIt != extractor.end() && regexIt->is_string() && nameIt->is_string() ) {
            if( std::regex_match( name, std::regex( regexIt->get< std::string >() ) ) ) {
                return nameIt->get< std::string >();
            }
        }
    }

    return common::optional< std::string >();
}

}  // namespace Zigbee
}  // namespace HubPlatform
