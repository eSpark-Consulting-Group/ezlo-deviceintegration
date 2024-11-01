#include "variant/variant.hpp"

template < typename... Ts >
using Variant = mpark::variant< Ts... >;
