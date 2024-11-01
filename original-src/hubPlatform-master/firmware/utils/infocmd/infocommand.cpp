#include "ui_info.h"
#include "bus/spread_bus.h"

static void bus_rx_cb( const bus_rx_data_t* rx_data ) {
    return;
}

int main( int argc, char* argv[] ) {
    if( argc < 2 ) {
        std::cerr << "Wrong params count: " << argc << ". Usage: ha-infocmd type [message]" << std::endl;
        return 1;
    }
    try {
        auto bus = createBus( "infocmd", []( const bus_rx_data_t* /*rx_data*/ ) {}, true, 1 );
        bus->join( "control_group" );
        bus->start();

        std::string type = argv[ 1 ];
        std::string message = argc >= 3 ? argv[ 2 ] : "";
        System::sendMessage( *bus, type, message );
    }
    catch( const std::runtime_error& error ) {
        return 1;
    }

    return 0;
}
