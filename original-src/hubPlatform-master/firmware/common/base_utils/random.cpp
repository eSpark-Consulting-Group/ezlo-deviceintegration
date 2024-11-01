#include "base_utils/random.h"

namespace Common {

Random::Random( int from, int to )
: randomDevice()
, seedGenerator( randomDevice() )
, distribution( from, to ) {}

int Random::operator()() {
    return distribution( seedGenerator );
}

}  // namespace Common
