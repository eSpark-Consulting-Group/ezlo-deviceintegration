#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace ExtensionsManager {

class TemplateNotFoundError : public std::runtime_error {
public:
    explicit TemplateNotFoundError( const std::string& templateName )
    : runtime_error{ "Template not found by name: " + templateName } {}
};

class FileNotFoundError : public std::runtime_error {
public:
    explicit FileNotFoundError( const std::string& fileId )
    : runtime_error{ "File not found by file Id: " + fileId } {}
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
