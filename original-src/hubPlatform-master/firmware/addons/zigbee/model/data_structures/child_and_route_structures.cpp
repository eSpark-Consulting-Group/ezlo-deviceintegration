#include "zigbee/model/data_structures/child_and_route_structures.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

std::string ChildData::typeToString() const {
    using namespace Resources::Fields::NetworkInfo;

    switch( type ) {
    case Type::Coordinator:
        return ChildInfo::Type::Coordinator;
    case Type::Router:
        return ChildInfo::Type::Router;
    case Type::EndDevice:
        return ChildInfo::Type::EndDevice;
    case Type::SleepyEndDevice:
        return ChildInfo::Type::SleepyEndDevice;
    default:
        return ChildInfo::Type::Unknown;
    }
}

std::string ChildData::timeoutToString() const {
    using namespace Resources::Fields::NetworkInfo;

    if( timeout == 0 ) {
        return ChildInfo::Timeout::TenSec;
    }
    else {
        return std::to_string( 2 << ( timeout - 1 ) ) + " " + ChildInfo::Timeout::Minutes;
    }
}

std::string RouteData::concentratorTypeToString() const {
    using namespace Resources::Fields::NetworkInfo;

    switch( concentratorType ) {
    case ConcentratorType::Low:
        return RouteInfo::ConcentratorType::Low;
    case ConcentratorType::High:
        return RouteInfo::ConcentratorType::High;
    default:
        return RouteInfo::ConcentratorType::Unknown;
    }
}

std::string RouteData::statusToString() const {
    using namespace Resources::Fields::NetworkInfo;

    switch( status ) {
    case Status::Active:
        return RouteInfo::Status::Active;
    case Status::Discovering:
        return RouteInfo::Status::Discovering;
    case Status::Unused:
        return RouteInfo::Status::Unused;
    default:
        return RouteInfo::Status::Unknown;
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
