#pragma once

#include <stdio.h>

// TODO: Temporary solution. Create logging system or use existing.
#define ZB_LOG_ERR( format, ... )        \
    do {                                 \
        printf( format, ##__VA_ARGS__ ); \
    } while( 0 )

#define ZB_LOG_DEBUG( format, ... ) ZB_LOG_ERR( format, ##__VA_ARGS__ )
#define ZB_LOG_INFO( format, ... ) ZB_LOG_ERR( format, ##__VA_ARGS__ )
#define ZB_LOG_WARN( format, ... ) ZB_LOG_ERR( format, ##__VA_ARGS__ )
