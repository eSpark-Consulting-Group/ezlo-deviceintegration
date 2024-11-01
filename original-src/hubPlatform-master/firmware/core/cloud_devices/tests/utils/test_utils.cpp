#include "vidoo4hub/tests/utils/test_utils.h"

#include <atomic>
#include <unistd.h>

namespace HubPlatform {
namespace UT {

std::string getUniqueID() {
    static std::atomic_int pid( getpid() );
    static std::atomic_uint id{ 0 };
    return "id_" + std::to_string( pid ) + "_" + std::to_string( ++id );
}

}  // namespace UT
}  // namespace HubPlatform
