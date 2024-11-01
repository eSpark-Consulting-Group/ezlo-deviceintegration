
#pragma once

namespace Uid {
namespace Errors {

class BusAlreadySet : public std::runtime_error {
public:
    BusAlreadySet()
    : runtime_error( "Bus singleton has already set." ) {}
};

class BusHasNotBeenInitialized : public std::runtime_error {
public:
    BusHasNotBeenInitialized()
    : runtime_error( "Bus singleton has not initialized." ) {}
};

}  // namespace Errors
}  // namespace Uid
