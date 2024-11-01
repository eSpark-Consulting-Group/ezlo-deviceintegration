#pragma once

#include "json_utils/json_utils.h"
#include "base_utils/optional.h"

namespace HubPlatform {
namespace Zigbee {

class NodeInterviewNamesExtractors {
public:
    static std::unique_ptr< NodeInterviewNamesExtractors > create( const Json& configurations );

    common::optional< std::string > getExtractedManufacturerName( const std::string& manufacturerName ) const;
    common::optional< std::string > getExtractedModelName( const std::string& modelName ) const;

private:
    NodeInterviewNamesExtractors( const Json& manufacturerNamesExtarctors, const Json& modelNamesExtarctors );

    static common::optional< std::string > getExtractedName( const std::string& name, const Json& extractors );

private:
    Json m_manufacturerNamesExtarctors;
    Json m_modelNamesExtarctors;
};

}  // namespace Zigbee
}  // namespace HubPlatform
