#pragma once

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

struct SynchronizationStatus {
    enum Enum { Synchronized, Pending, Failed };

public:
    static Enum fromString( const std::string& string );

    static const char* toString( Enum value );
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
