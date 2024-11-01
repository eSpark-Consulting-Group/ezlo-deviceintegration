#pragma once

#include <cctype>

namespace HubPlatform {
namespace Common {

inline int verrevcmp( const char* val, const char* ref ) {
    /* assume ascii; warning: evaluates x multiple times! */
    auto order = []( const char x ) -> int {
        return x == '~' ? -1 : isdigit( x ) ? 0 : !x ? 0 : isalpha( x ) ? x : x + 256;
    };

    if( !val )
        val = "";
    if( !ref )
        ref = "";

    while( *val || *ref ) {
        int first_diff = 0;

        while( ( *val && !isdigit( *val ) ) || ( *ref && !isdigit( *ref ) ) ) {
            int vc = order( *val ), rc = order( *ref );
            if( vc != rc )
                return vc - rc;
            val++;
            ref++;
        }

        while( *val == '0' )
            val++;
        while( *ref == '0' )
            ref++;
        while( isdigit( *val ) && isdigit( *ref ) ) {
            if( !first_diff )
                first_diff = *val - *ref;
            val++;
            ref++;
        }
        if( isdigit( *val ) )
            return 1;
        if( isdigit( *ref ) )
            return -1;
        if( first_diff )
            return first_diff;
    }
    return 0;
}

}  // namespace Common
}  // namespace HubPlatform
