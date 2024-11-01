#pragma once

#include <memory>

class IDbClient;

namespace HubPlatform {
namespace CoreD {
namespace Model {

class Configuration;

using ConfigUPtr = std::unique_ptr< Configuration >;

namespace ConfigurationFactory {

ConfigUPtr createConfiguration( const IDbClient& dbClient );

}  // namespace ConfigurationFactory
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
