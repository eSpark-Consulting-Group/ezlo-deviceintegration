#pragma once

#include <stdexcept>

namespace SceneEngine {

class EventException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}  // namespace SceneEngine
