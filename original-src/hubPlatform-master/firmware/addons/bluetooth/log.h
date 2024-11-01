#pragma once

#include "log_utils/ezlo_log.h"

// TODO: Temporary solution. Create logging system or use existing.
#define LOG( log, format, ... )                \
    do {                                       \
        char msg[ 2048 ];                      \
        sprintf( msg, format, ##__VA_ARGS__ ); \
        log( msg );                            \
    } while( 0 )

#define LOG_ERROR( format, ... ) LOG( Log::error, format, ##__VA_ARGS__ )
#define LOG_INFO( format, ... ) LOG( Log::info, format, ##__VA_ARGS__ )
#define LOG_WARN( format, ... ) LOG( Log::warn, format, ##__VA_ARGS__ )
#define LOG_DEBUG( format, ... ) LOG( Log::debug, format, ##__VA_ARGS__ )
