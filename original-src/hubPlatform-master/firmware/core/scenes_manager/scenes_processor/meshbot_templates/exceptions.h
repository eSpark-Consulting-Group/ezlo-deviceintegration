#pragma once

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {

class MeshbotTemplatesError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit MeshbotTemplatesError( const std::string& msg )
    : runtime_error( msg ){};
};

class MeshbotTemplateParsingError : public MeshbotTemplatesError {
public:
    explicit MeshbotTemplateParsingError( const std::string& msg )
    : MeshbotTemplatesError( msg ){};
};

}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
