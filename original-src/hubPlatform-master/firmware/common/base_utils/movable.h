#pragma once

namespace common {
class Movable {
protected:
    Movable() = default;

public:
    Movable( const Movable& ) = delete;
    Movable& operator=( const Movable& ) = delete;

public:
    Movable( Movable&& ) = default;
    Movable& operator=( Movable&& ) = default;
};
}  // namespace common
