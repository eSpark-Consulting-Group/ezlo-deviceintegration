
#include "uid/ui_service.h"

#include "service_interfaces/daemon_service.h"

// NmaClient::g_bEnableNmaClientStatsLogging = true;
//
//#if( defined __TINY_WS_OFFER_DETAILED_NLWS_CONFIGURATION_OPTIONS__ )
//#endif  // ( defined __TINY_WS_OFFER_DETAILED_NLWS_CONFIGURATION_OPTIONS__ )

int main( int argc, char** argv ) {
    using namespace HubPlatform;
    return Services::DaemonService< Uid::Service >().run( argc, argv );
}
