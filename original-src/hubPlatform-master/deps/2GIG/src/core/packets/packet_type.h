#pragma once

#include <cassert>

namespace TwoGig {

struct PacketType {
    enum Enum { Regular = 0x55, Supervisory = 0x37 };
};

}  // namespace TwoGig
