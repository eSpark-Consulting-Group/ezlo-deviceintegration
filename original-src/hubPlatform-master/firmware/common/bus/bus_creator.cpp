#include "bus/spread_bus.h"

BaseBusPtr createBus( const std::string& privateName,
                      const RxCallBack& rxCallBack,
                      bool silent,
                      int connectionAttempts ) {
    return BaseBusPtr( new SpreadBus( privateName, rxCallBack, silent, connectionAttempts ), []( BaseBus* spread ) {
        spread->stop();
        delete spread;
    } );
}
