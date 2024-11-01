#pragma once
#include "clock_interface.h"

namespace SceneEngine {

class DaylightSavingsTime {
public:
    static bool hasDSTChange( IClock& clocks );
};

}  // namespace SceneEngine
