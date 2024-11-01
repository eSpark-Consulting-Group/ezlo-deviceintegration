#include "ezlo_common.h"

#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <tiny/tiny_consolecolors.h>
#include <tiny/tiny_utils.h>
#include <atomic>

#include <fstream>

namespace ezlo {

namespace common {

namespace APIs {

template < typename clock_type_t = std::chrono::high_resolution_clock >
inline time_t clock_2_time_t( const typename clock_type_t::time_point& ptTime ) {
#if( defined __BUILDING_4_MAC_OS_X__ )
    time_t tt = std::chrono::time_point_cast< std::chrono::seconds >( ptTime ).time_since_epoch().count();
    return tt;
#else   // (defined __BUILDING_4_MAC_OS_X__)
    time_t tt = clock_type_t::to_time_t( ptTime );
    return tt;
#endif  // (defined __BUILDING_4_MAC_OS_X__)
}

void json_set_time_mark( const std::chrono::high_resolution_clock::time_point& ptTime,
                         JsonObject& jo,
                         const char* strAttrNameLocal,  // = "modified"     // if nullptr, do not add attribute
                         const char* strAttrNameUTC,    // = "modified_utc" // if nullptr, do not add attribute
                         bool isDaysInsteadOfYMD        // = false
) {
    bool hHaveLocal = ( strAttrNameLocal != nullptr && strAttrNameLocal[ 0 ] != '\0' ) ? true : false;
    bool hHaveUTC = ( strAttrNameUTC != nullptr && strAttrNameUTC[ 0 ] != '\0' ) ? true : false;
    if( !( hHaveLocal || hHaveUTC ) )
        return;
    if( hHaveLocal )
        jo[ std::string( strAttrNameLocal ) ] =
            cc::time2string( ptTime, false, isDaysInsteadOfYMD, false );  // not colored
    if( hHaveUTC )
        jo[ std::string( strAttrNameUTC ) ] =
            cc::time2string( ptTime, true, isDaysInsteadOfYMD, false );  // not colored
}
void json_set_now_mark( JsonObject& jo,
                        const char* strAttrNameLocal,  // = "timestamp"     // if nullptr, do not add attribute
                        const char* strAttrNameUTC,    // = "timestamp_utc" // if nullptr, do not add attribute
                        bool isDaysInsteadOfYMD        // = false
) {
    std::chrono::high_resolution_clock::time_point ptTimeNow = std::chrono::high_resolution_clock::now();
    json_set_time_mark( ptTimeNow, jo, strAttrNameLocal, strAttrNameUTC, isDaysInsteadOfYMD );
}

bool extract_set_of_strings_from_json_array( const JsonArray& jarr, std::set< std::string >& a_set ) {
    try {
        a_set.clear();
        if( !jarr.is_array() ) {
            return false;
        }
        size_t cnt = jarr.size();
        for( size_t i = 0; i < cnt; ++i ) {
            const JsonObject& jo = jarr[ i ];
            std::string s = jo.get< std::string >();
            a_set.insert( s );
        }
        return true;
    }
    catch( ... ) {
        a_set.clear();
        return false;
    }
}
bool extract_set_of_strings_from_json_parms( const char* strNameArray,
                                             const char* strNameSingle,
                                             const JsonObject& params,
                                             std::set< std::string >& a_set,
                                             bool bNotPresentPropsOKay ) {
    if( strNameArray == nullptr || strNameArray[ 0 ] == '\0' )
        return false;
    if( strNameSingle == nullptr || strNameSingle[ 0 ] == '\0' )
        strNameSingle = strNameArray;
    try {
        a_set.clear();
        // try find array
        if( params.find( strNameArray ) != params.end() ) {
            const JsonObject& jarr = params[ std::string( strNameArray ) ];
            if( jarr.is_array() ) {
                if( !extract_set_of_strings_from_json_array( jarr, a_set ) )
                    return false;
                return true;
            }
        }  // if( params.find("streams") == params.end() )
        // OKay, try find single
        a_set.clear();
        if( params.find( strNameSingle ) != params.end() ) {
            std::string strUrlSrc = params[ std::string( strNameSingle ) ];
            if( strUrlSrc.empty() )
                return false;
            a_set.insert( strUrlSrc );
            return true;
        }  // if( params.find(strNameSingle) == params.end() )
        if( bNotPresentPropsOKay )
            return true;
        return false;
    }
    catch( ... ) {
        a_set.clear();
        return false;
    }
}

bool file2string( const char* strFileName, std::string& s ) {
    s.clear();
    if( strFileName == NULL || strFileName[ 0 ] == '\0' )
        return false;
    try {
        std::ifstream t( strFileName );
        t.seekg( 0, std::ios::end );
        s.reserve( size_t( t.tellg() ) );
        t.seekg( 0, std::ios::beg );
        s.assign( ( std::istreambuf_iterator< char >( t ) ), std::istreambuf_iterator< char >() );
        return true;
    }
    catch( const std::exception& ) {
        s.clear();
        return false;
    }
    catch( ... ) {
        s.clear();
        throw;
    }
}
std::string file2string( const char* strFileName ) {
    std::string s;
    if( !file2string( strFileName, s ) )
        s.clear();
    return s;
}
bool string2file( const char* strFileName, const char* s ) {
    if( strFileName == NULL || strFileName[ 0 ] == '\0' )
        return false;
    try {
        size_t len = ( s != nullptr && s[ 0 ] != '\0' ) ? ::strlen( s ) : 0;
        std::ofstream t( strFileName );
        if( len > 0 )
            t.write( s, std::streamsize( len ) );
        t.close();
        return true;
    }
    catch( const std::exception& ) {
        return false;
    }
    catch( ... ) {
        throw;
    }
}
bool string2file( const char* strFileName, const std::string& s ) {
    return string2file( strFileName, s.empty() ? "" : s.c_str() );
}
bool file2json( const char* strFileName, JsonObject& jo ) {
    jo = JSON_EMPTY_OBJ;
    if( strFileName == NULL || strFileName[ 0 ] == '\0' )
        return false;
    try {
        std::ifstream t( strFileName );
        t >> jo;
        return true;
    }
    catch( const std::exception& ) {
        jo = JSON_EMPTY_OBJ;
        return false;
    }
    catch( ... ) {
        throw;
    }
}
bool json2file( const char* strFileName, const JsonObject& jo ) {
    if( strFileName == NULL || strFileName[ 0 ] == '\0' )
        return false;
    try {
        std::ofstream t( strFileName );
        t << jo.dump( 4 );
        return true;
    }
    catch( const std::exception& ) {
        return false;
    }
    catch( ... ) {
        throw;
    }
}
std::vector< unsigned char > file2vec( const char* strFileName ) {
    std::vector< unsigned char > vec;
    if( strFileName == NULL || strFileName[ 0 ] == '\0' )
        return vec;
    FILE* f = ::fopen( strFileName, "rb" );
    if( !f )
        return vec;
    while( !::feof( f ) ) {
        unsigned char c;
        ::fread( &c, sizeof( c ), 1, f );
        vec.push_back( c );
    }
    ::fclose( f );
    return vec;
}
std::string file2base64( const char* strFileName ) {
    std::vector< unsigned char > vec = file2vec( strFileName );
    const std::string s = tiny::tools::base64::encode( &vec[ 0 ], vec.size() );
    return s;
}
std::string base64Encode( const std::vector< unsigned char >& vec ) {
    std::string s = tiny::tools::base64::encode( &vec[ 0 ], vec.size() );
    return s;
}
std::string base64Decode( const std::string& str ) {
    std::string result = tiny::tools::base64::decode( str );
    return result;
}
std::string encode_command_line_argument( const char* strArg ) {
    return ( strArg != nullptr && strArg[ 0 ] != '\0' ) ? tiny::tools::escapeJSON( strArg ) : "";
}
std::string decode_command_line_argument( const char* strArg ) {
    return ( strArg != nullptr && strArg[ 0 ] != '\0' ) ? tiny::tools::unescapeJSON( strArg ) : "";
}

std::list< std::string > split_string( const char* text, const char* delims ) {
    std::string text_( text ), delims_( delims );
    return split_string( text_, delims_ );
}
std::list< std::string > split_string( const std::string& text, const std::string& delims ) {
    std::list< std::string > tokens;
    std::size_t start = text.find_first_not_of( delims ), end = 0;
    while( ( end = text.find_first_of( delims, start ) ) != std::string::npos ) {
        tokens.push_back( text.substr( start, end - start ) );
        start = text.find_first_not_of( delims, end );
    }
    if( start != std::string::npos )
        tokens.push_back( text.substr( start ) );
    return tokens;
}

};  // namespace APIs

};  // namespace common
};  // namespace ezlo
