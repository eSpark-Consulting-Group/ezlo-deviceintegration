#include "cored/model/setting/synchronization_status.h"
#include "cored/model/exceptions.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

SynchronizationStatus::Enum SynchronizationStatus::fromString( const std::string& string ) {
    if( string == "synced" )
        return Enum::Synchronized;

    if( string == "pending" )
        return Enum::Pending;

    if( string == "failed" )
        return Enum::Failed;

    throw CoreDModelError{ "Unexpected value for SynchronizationStatus: " + string };
}

const char* SynchronizationStatus::toString( Enum value ) {
    switch( value ) {
    case Enum::Synchronized:
        return "synced";

    case Enum::Pending:
        return "pending";

    case Enum::Failed:
        return "failed";

    default:
        return "unknown";
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
