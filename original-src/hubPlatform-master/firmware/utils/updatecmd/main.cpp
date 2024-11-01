#include "updatecmd/update_daemon.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return UpdateCommand::UpdateDaemon().run( argc, argv );
}
