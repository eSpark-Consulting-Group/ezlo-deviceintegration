#pragma once

namespace common {
class NonCopyable {
protected:
    NonCopyable() = default;

public:
    NonCopyable( const NonCopyable& ) = delete;
    NonCopyable& operator=( const NonCopyable& ) = delete;
    NonCopyable( NonCopyable&& ) = delete;
    NonCopyable& operator=( NonCopyable&& ) = delete;
};
}  // namespace common
