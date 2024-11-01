#pragma once

#include "bus/base_bus.h"

#include <functional>

#include <string>

namespace HubPlatform {
namespace Services {

class BusWrapper final {
public:
    template < typename BusType >
    struct BusTypeTag {};

public:
    BusWrapper( int connectionAttempts = 10 );
    ~BusWrapper();

    template < typename BusType >
    BusWrapper( BusTypeTag< BusType >, int connectionAttempts = 10 )
    : m_bus( nullptr, nullptr )
    , m_connectionAttempts( connectionAttempts )
    , m_createBus( []( const std::string& privateName,
                       const RxCallBack& rxCallBack,
                       bool silent,
                       int connectionAttempts ) -> BaseBusPtr {
        return BaseBusPtr( new BusType( privateName, rxCallBack, silent, connectionAttempts ), []( BaseBus* spread ) {
            spread->stop();
            delete spread;
        } );
    } ) {}

    void setUp( const std::string& name, const RxCallBack& busCallBack );
    void tearDown();

    const BaseBus& getBus() const;
    BaseBus& getBus();

private:
    BaseBusPtr m_bus;
    int m_connectionAttempts;

    // Note: for test purpose we have custom bus create functor.
    using BusCreationFunction = decltype( &createBus );
    BusCreationFunction m_createBus;
};
}  // namespace Services
}  // namespace HubPlatform
