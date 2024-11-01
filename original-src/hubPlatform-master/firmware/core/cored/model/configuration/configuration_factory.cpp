#include "cored/model/configuration/configuration_factory.h"

#include "service_interfaces/service.h"
#include "cored/model/configuration/configuration_impl.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {
namespace ConfigurationFactory {

ConfigUPtr createConfiguration( const IDbClient& dbClient ) {
    return std::make_unique< ConfigurationImpl >( dbClient );
}

}  // namespace ConfigurationFactory
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
