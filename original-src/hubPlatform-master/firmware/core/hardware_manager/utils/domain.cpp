#include "hardware_manager/utils/domain.h"
#include <math.h>

float Domain::normalize( int x, int max ) {
    return ( expf( ( (float) x ) / max ) - 1.0 ) / ( M_E - 1.0 );
}