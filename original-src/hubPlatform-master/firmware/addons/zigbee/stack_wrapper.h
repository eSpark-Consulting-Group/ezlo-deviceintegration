#pragma once

#include <functional>

namespace ZigbeeStackWrapper {

extern const int RESULT_SUCCESS;

void resetAndInitNCP();
int init( int argc, char** argv );
int initForBackupRestore( int argc, char** argv );
void mainLoop();
void mainLoopTick();
void setMainLoopTickHandler( std::function< void( void ) > handlerFunction );

bool getStackVersion( int& returnCode );
void getNetworkParameters();

// 0x00 - disable joining, 0xFF - enable joining, other value - joining period in seconds
void permitJoining( const unsigned char interval = 0xFF );
void resetToDefaults();
void startNewNetwork();

}  // namespace ZigbeeStackWrapper
