#include <tiny/tiny_utils.h>
#include <tiny/tiny_consolecolors.h>

#include <ctype.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <inttypes.h>
#include <fcntl.h>
#include <assert.h>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <signal.h>

#if( !defined( __HUB_BUILDING_TARGET_ARCH_MIPS__ ) && !defined( _LIBCPP_HAS_MUSL_LIBC ) )
#include <execinfo.h>
#endif  // __HUB_BUILDING_TARGET_ARCH_MIPS__

extern "C" {
#ifdef WIN32
#include <Rpc.h>
#else
//		#include <uuid/uuid.h>
#endif
}

#if( defined __BUILDING_4_MAC_OS_X__ )
#include <mach/mach_init.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/vm_map.h>
#endif

namespace tiny {
namespace tools {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

string_list_t split( const std::string& s, char delim ) {
    std::stringstream ss( s );
    std::string item;
    string_list_t lst;
    while( std::getline( ss, item, delim ) )
        lst.push_back( item );
    return lst;
}
string_list_t split( const std::string& s, const char* delimiters ) {
    string_list_t lst;
    std::stringstream ss( s );
    std::string line;
    while( std::getline( ss, line ) ) {
        std::size_t prev = 0, pos;
        while( ( pos = line.find_first_of( delimiters, prev ) ) != std::string::npos ) {
            if( pos > prev )
                lst.push_back( line.substr( prev, pos - prev ) );
            prev = pos + 1;
        }
        if( prev < line.length() )
            lst.push_back( line.substr( prev, std::string::npos ) );
    }
    return lst;
}

void replace_all( std::string& s, const std::string& f, const std::string& r ) {  // replace (in place)
    for( size_t pos = 0;; pos += r.length() ) {
        pos = s.find( f, pos );
        if( pos == std::string::npos )
            break;
        s.erase( pos, f.length() );
        s.insert( pos, r );
    }
}
std::string replace_all_copy( const std::string& s, const std::string& f, const std::string& r ) {  // replace (copying)
    std::string ret = s;
    replace_all( ret, f, r );
    return ret;
}

// Note, we should pass value representable as unsigned char to std::tolower(int), std::toupper(int) etc.
//       Otherwise - undefuned behavior.
char to_upper( char c ) {
    return static_cast< char >( std::toupper( static_cast< unsigned char >( c ) ) );
}

char to_lower( char c ) {
    return static_cast< char >( std::tolower( static_cast< unsigned char >( c ) ) );
}

std::string to_upper( const std::string& strSrc ) {
    std::string strDst;
    std::transform( strSrc.begin(),
                    strSrc.end(),
                    std::back_inserter< std::string >( strDst ),
                    static_cast< char ( * )( char ) >( to_upper ) );
    return strDst;
}
std::string to_lower( const std::string& strSrc ) {
    std::string strDst;
    std::transform( strSrc.begin(),
                    strSrc.end(),
                    std::back_inserter< std::string >( strDst ),
                    static_cast< char ( * )( char ) >( to_lower ) );
    return strDst;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

string_list_t enum_dir( const char* strDirPath, bool bDirs, bool bFiles ) {
    string_list_t lstDirEntries;
    if( !( bDirs || bFiles ) )
        return lstDirEntries;
    DIR* dir = ::opendir( strDirPath ? strDirPath : "/" );
    if( dir == nullptr )
        return lstDirEntries;
    struct dirent* entry = nullptr;
    while( ( entry = ::readdir( dir ) ) != nullptr ) {
        if( entry->d_type == DT_DIR ) {
            // char path[1024];
            if(::strcmp( entry->d_name, "." ) == 0 || ::strcmp( entry->d_name, ".." ) == 0 )
                continue;
            //::snprintf( path, sizeof(path), "%s/%s", strDirPath, entry->d_name );
            //::printf( "%*s[%s]\n", indent, "", entry->d_name );
            // listdir( path, indent + 2 );
            if( bDirs )
                lstDirEntries.push_back( entry->d_name );
        }
        else {
            //::printf("%*s- %s\n", indent, "", entry->d_name);
            if( bFiles )
                lstDirEntries.push_back( entry->d_name );
        }
    }
    ::closedir( dir );
    return lstDirEntries;
}

bool file_exists( const std::string& filename,
                  size_t* p_size  // = nullptr
) {
    struct stat st;
    ::memset( &st, 0, sizeof( struct stat ) );
    if( p_size != nullptr )
        ( *p_size ) = 0;
    if(::stat( filename.c_str(), &st ) != -1 ) {
        if( p_size != nullptr )
            ( *p_size ) = st.st_size;
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string escapeJSON( const std::string& input ) {
    std::string output;
    if( input.empty() )
        return output;
    output.reserve( input.length() );
    std::string::size_type i, cnt = input.length();
    for( i = 0; i < cnt; ++i ) {
        switch( input[ i ] ) {
        case '"':
            output += "\\\"";
            break;
        case '/':
            output += "\\/";
            break;
        case '\b':
            output += "\\b";
            break;
        case '\f':
            output += "\\f";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        case '\\':
            output += "\\\\";
            break;
        default:
            output += input[ i ];
            break;
        }
    }
    return output;
}

std::string unescapeJSON( const std::string& input ) {
    std::string output;
    if( input.empty() )
        return output;
    enum class State { ESCAPED, UNESCAPED };
    State s = State::UNESCAPED;
    output.reserve( input.length() );
    std::string::size_type i, cnt = input.length();
    for( i = 0; i < cnt; ++i ) {
        switch( s ) {
        case State::ESCAPED:
            switch( input[ i ] ) {
            case '"':
                output += '\"';
                break;
            case '/':
                output += '/';
                break;
            case 'b':
                output += '\b';
                break;
            case 'f':
                output += '\f';
                break;
            case 'n':
                output += '\n';
                break;
            case 'r':
                output += '\r';
                break;
            case 't':
                output += '\t';
                break;
            case '\\':
                output += '\\';
                break;
            default:
                output += input[ i ];
                break;
            }
            s = State::UNESCAPED;
            break;
        case State::UNESCAPED:
            switch( input[ i ] ) {
            case '\\':
                s = State::ESCAPED;
                break;
            default:
                output += input[ i ];
                break;
            }
            break;
        }
    }
    return output;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool to_bool( const std::string& strValue ) {
    if( strValue.empty() )
        return false;
    std::string s = to_lower( strValue );
    s.erase( s.begin(),
             std::find_if( s.begin(), s.end(), []( int chr ) { return !std::isspace( chr ); } ) );  // trim left
    s.erase( std::find_if( s.rbegin(), s.rend(), []( int chr ) { return !std::isspace( chr ); } ).base(),
             s.end() );  // trim right
    //  s.erase( s.begin(), std::find_if( s.begin(), s.end(), std::not1( std::ptr_fun<int,int>( std::isspace ) ) ) ); //
    //  trim left s.erase(std::find_if( s.rbegin(), s.rend(), std::not1( std::ptr_fun<int,int>( std::isspace ) )
    //  ).base(), s.end() ); // trim right
    if( s.empty() )
        return false;
    char chr = s[ 0 ];
    switch( chr ) {
    case 't':
    case 'y':
        return true;
    case 'f':
    case 'n':
        return false;
    }
    if( std::isdigit( chr ) ) {
        double val = std::atof( s.c_str() );
        if( val != 0.0 )
            return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string nanoseconds_2_lifetime_str( uint64_t ns, bool isColored /*= false*/ ) {
    uint64_t microseconds = ns / 1000;
    ns %= 1000;
    uint64_t milliseconds = microseconds / 1000;
    microseconds %= 1000;
    uint64_t seconds = milliseconds / 1000;
    milliseconds %= 1000;
    uint64_t minutes = seconds / 60;
    seconds %= 60;
    uint64_t hours = minutes / 60;
    minutes %= 60;
    uint64_t days = hours / 24;
    hours %= 24;
    std::string strDays, strHours, strMinutes, strSeconds, strMilliSeconds, strMicroSeconds, strNanoSeconds;
    std::string strDaysSuffix, strHoursSuffix, strMinutesSuffix, strSecondsSuffix, strMilliSecondsSuffix,
        strMicroSecondsSuffix, strNanoSecondsSuffix;
    std::stringstream ss;
    bool separatorRequired = false;

    auto getSeparator = [&separatorRequired]() { return separatorRequired ? " " : ""; };

    if( days > 0 ) {
        strDays = tiny::tools::format( "%" PRId64, days );
        strDaysSuffix = "d";
        if( isColored ) {
            strDays = cc::note( strDays );
            strDaysSuffix = cc::debug( strDaysSuffix );
        }
        ss << strDays << strDaysSuffix;
        separatorRequired = true;
    }
    if( hours > 0 || days > 0 ) {
        strHours = tiny::tools::format( "%02d", int( hours ) );
        strHoursSuffix = "h";
        if( isColored ) {
            strHours = cc::note( strHours );
            strHoursSuffix = cc::debug( strHoursSuffix );
        }
        ss << getSeparator() << strHours << strHoursSuffix;
    }
    if( minutes > 0 || hours > 0 || days > 0 ) {
        strMinutes = tiny::tools::format( "%02d", int( minutes ) );
        strMinutesSuffix = "m";
        if( isColored ) {
            strMinutes = cc::note( strMinutes );
            strMinutesSuffix = cc::debug( strMinutesSuffix );
        }
        ss << getSeparator() << strMinutes << strMinutesSuffix;
        separatorRequired = true;
    }
    if( seconds > 0 || minutes > 0 || hours > 0 || days > 0 ) {
        strSeconds = tiny::tools::format( "%02d", int( seconds ) );
        strSecondsSuffix = "s";
        if( isColored ) {
            strSeconds = cc::note( strSeconds );
            strSecondsSuffix = cc::debug( strSecondsSuffix );
        }
        ss << getSeparator() << strSeconds << strSecondsSuffix;
        separatorRequired = true;
    }
    if( milliseconds > 0 || seconds > 0 || minutes > 0 || hours > 0 || days > 0 ) {
        strMilliSeconds = tiny::tools::format( "%03d", int( milliseconds ) );
        strMilliSecondsSuffix = "ms";
        if( isColored ) {
            strMilliSeconds = cc::note( strMilliSeconds );
            strMilliSecondsSuffix = cc::debug( strMilliSecondsSuffix );
        }
        ss << getSeparator() << strMilliSeconds << strMilliSecondsSuffix;
        separatorRequired = true;
    }
    if( microseconds > 0 || milliseconds > 0 || seconds > 0 || minutes > 0 || hours > 0 || days > 0 ) {
        strMicroSeconds = tiny::tools::format( "%03d", int( microseconds ) );
        strMicroSecondsSuffix = "μs";
        if( isColored ) {
            strMicroSeconds = cc::note( strMicroSeconds );
            strMicroSecondsSuffix = cc::debug( strMicroSecondsSuffix );
        }
        ss << getSeparator() << strMicroSeconds << strMicroSecondsSuffix;
        separatorRequired = true;
    }
    if( ns > 0 || microseconds > 0 || milliseconds > 0 || seconds > 0 || minutes > 0 || hours > 0 || days > 0 ) {
        strNanoSeconds = tiny::tools::format( "%03d", int( ns ) );
        strNanoSecondsSuffix = "ns";
        if( isColored ) {
            strNanoSeconds = cc::note( strNanoSeconds );
            strNanoSecondsSuffix = cc::debug( strNanoSecondsSuffix );
        }
        ss << getSeparator() << strNanoSeconds << strNanoSecondsSuffix;
    }
    return ss.str();
}

size_t cpu_count() {
    static const size_t g_cntCPUs = size_t(::sysconf( _SC_NPROCESSORS_ONLN ) );
    return g_cntCPUs;
}

namespace base64 {

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";
static inline bool is_base64( unsigned char c ) {
    return ( isalnum( c ) || ( c == '+' ) || ( c == '/' ) );
}

std::string encode( unsigned char const* bytes_to_encode, size_t in_len ) {
    std::string ret;
    size_t i = 0, j = 0;
    unsigned char char_array_3[ 3 ], char_array_4[ 4 ];
    while( in_len-- ) {
        char_array_3[ i++ ] = *( bytes_to_encode++ );
        if( i == 3 ) {
            char_array_4[ 0 ] = ( char_array_3[ 0 ] & 0xfc ) >> 2;
            char_array_4[ 1 ] = ( ( char_array_3[ 0 ] & 0x03 ) << 4 ) + ( ( char_array_3[ 1 ] & 0xf0 ) >> 4 );
            char_array_4[ 2 ] = ( ( char_array_3[ 1 ] & 0x0f ) << 2 ) + ( ( char_array_3[ 2 ] & 0xc0 ) >> 6 );
            char_array_4[ 3 ] = char_array_3[ 2 ] & 0x3f;
            for( i = 0; ( i < 4 ); i++ )
                ret += base64_chars[ char_array_4[ i ] ];
            i = 0;
        }
    }
    if( i ) {
        for( j = i; j < 3; j++ )
            char_array_3[ j ] = '\0';
        char_array_4[ 0 ] = ( char_array_3[ 0 ] & 0xfc ) >> 2;
        char_array_4[ 1 ] = ( ( char_array_3[ 0 ] & 0x03 ) << 4 ) + ( ( char_array_3[ 1 ] & 0xf0 ) >> 4 );
        char_array_4[ 2 ] = ( ( char_array_3[ 1 ] & 0x0f ) << 2 ) + ( ( char_array_3[ 2 ] & 0xc0 ) >> 6 );
        char_array_4[ 3 ] = char_array_3[ 2 ] & 0x3f;
        for( j = 0; ( j < i + 1 ); j++ )
            ret += base64_chars[ char_array_4[ j ] ];
        while( ( i++ < 3 ) )
            ret += '=';
    }
    return ret;
}

std::string decode( std::string const& encoded_string ) {
    size_t in_len = encoded_string.size();
    size_t i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[ 4 ], char_array_3[ 3 ];
    std::string ret;
    while( in_len-- && ( encoded_string[ in_ ] != '=' ) && is_base64( encoded_string[ in_ ] ) ) {
        char_array_4[ i++ ] = encoded_string[ in_ ];
        in_++;
        if( i == 4 ) {
            for( i = 0; i < 4; i++ )
                char_array_4[ i ] = base64_chars.find( char_array_4[ i ] );
            char_array_3[ 0 ] = ( char_array_4[ 0 ] << 2 ) + ( ( char_array_4[ 1 ] & 0x30 ) >> 4 );
            char_array_3[ 1 ] = ( ( char_array_4[ 1 ] & 0xf ) << 4 ) + ( ( char_array_4[ 2 ] & 0x3c ) >> 2 );
            char_array_3[ 2 ] = ( ( char_array_4[ 2 ] & 0x3 ) << 6 ) + char_array_4[ 3 ];
            for( i = 0; ( i < 3 ); i++ )
                ret += char_array_3[ i ];
            i = 0;
        }
    }
    if( i ) {
        for( j = i; j < 4; j++ )
            char_array_4[ j ] = 0;
        for( j = 0; j < 4; j++ )
            char_array_4[ j ] = base64_chars.find( char_array_4[ j ] );
        char_array_3[ 0 ] = ( char_array_4[ 0 ] << 2 ) + ( ( char_array_4[ 1 ] & 0x30 ) >> 4 );
        char_array_3[ 1 ] = ( ( char_array_4[ 1 ] & 0xf ) << 4 ) + ( ( char_array_4[ 2 ] & 0x3c ) >> 2 );
        char_array_3[ 2 ] = ( ( char_array_4[ 2 ] & 0x3 ) << 6 ) + char_array_4[ 3 ];
        for( j = 0; ( j < i - 1 ); j++ )
            ret += char_array_3[ j ];
    }
    return ret;
}
};  // namespace base64

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

md5::md5() {
    init();
}
void md5::update( uint1* input, size_t input_length ) {
    // MD5 block update operation. Continues an MD5 message-digest operation, processing another message block, and
    // updating the context.
    uint4 input_index, buffer_index;
    uint4 buffer_space;  // how much space is left in buffer
    if( finalized )      // so we can't update
        throw std::runtime_error( "md5::update() cannot update a finalized digest" );
    // compute number of bytes mod 64
    buffer_index = (unsigned int) ( ( count[ 0 ] >> 3 ) & 0x3F );
    // update number of bits
    if( ( count[ 0 ] += ( (uint4) input_length << 3 ) ) < ( (uint4) input_length << 3 ) )
        count[ 1 ]++;
    count[ 1 ] += ( (uint4) input_length >> 29 );
    buffer_space = 64 - buffer_index;  // how much space is left in buffer
    // transform as many times as possible.
    if( input_length >= buffer_space ) {  // ie. we have enough to fill the buffer
        // fill the rest of the buffer and transform
        memcpy( buf + buffer_index, input, buffer_space );
        transform( buf );
        // now, transform each 64-byte piece of the input, bypassing the buffer
        for( input_index = buffer_space; input_index + 63 < input_length; input_index += 64 )
            transform( input + input_index );
        buffer_index = 0;  // so we can buffer remaining
    }
    else
        input_index = 0;  // so we can buffer the whole input
    // and here we do the buffering:
    memcpy( buf + buffer_index, input + input_index, input_length - input_index );
}
void md5::update( FILE* f ) {
    // MD5 update for files. Like above, except that it works on files (and uses above as a primitive.)
    if( !f )
        return;
    unsigned char buf[ 1024 ];
    int len;
    while( ( len = fread( buf, 1, 1024, f ) ) != 0 )
        update( buf, len );
    fclose( f );
}
void md5::update( std::istream& stream ) {
    // MD5 update for istreams. Like update for files; see above.
    unsigned char buf[ 1024 ];
    int len;
    while( stream.good() ) {
        stream.read( (char*) buf, 1024 );  // note that return value of read is unusable.
        len = (int) stream.gcount();
        update( buf, len );
    }
}
void md5::update( std::ifstream& stream ) {
    // MD5 update for ifstreams. Like update for files; see above.
    unsigned char buf[ 1024 ];
    int len;
    while( stream.good() ) {
        stream.read( (char*) buf, 1024 );  // note that return value of read is unusable.
        len = (int) stream.gcount();
        update( buf, len );
    }
}
void md5::finalize() {
    // MD5 finalization. Ends an MD5 message-digest operation, writing the the message digest and zeroizing the context.
    unsigned char bits[ 8 ];
    unsigned int index, padLen;
    static uint1 PADDING[ 64 ] = { 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    if( finalized )
        throw std::runtime_error( "md5::finalize: already finalized this digest" );
    // save number of bits
    encode( bits, count, 8 );
    // pad out to 56 mod 64.
    index = ( uint4 )( ( count[ 0 ] >> 3 ) & 0x3f );
    padLen = ( index < 56 ) ? ( 56 - index ) : ( 120 - index );
    update( PADDING, padLen );
    update( bits, 8 );                // append length (before padding)
    encode( digest, state, 16 );      // store state in digest
    memset( buf, 0, sizeof( buf ) );  // zeroize sensitive information
    finalized = 1;
}
md5::md5( FILE* f ) {
    init();  // must be called be all constructors
    update( f );
    finalize();
}
md5::md5( std::istream& stream ) {
    init();  // must called by all constructors
    update( stream );
    finalize();
}
md5::md5( std::ifstream& stream ) {
    init();  // must called by all constructors
    update( stream );
    finalize();
}
md5::RawDigest md5::raw_digest() {
    RawDigest s( 16 );
    if( !finalized ) {
        // std::cerr << "md5::raw_digest: cannot get digest if you haven't finalized the digest" << std::endl;
        s[ 0 ] = '\0';
        return s;
    }
    memcpy( s.data(), digest, 16 );
    return s;
}
std::string md5::hex_digest() {
    std::string result;
    result.reserve( 33 );
    if( !finalized ) {
        // std::cerr << "md5::hex_digest: cannot get digest if you haven't finalized the digest!" << std::endl;
        return result;
    }
    char hexChar[ 16 ];
    for( int i = 0; i < 16; i++ ) {
        sprintf( hexChar, "%02x", digest[ i ] );
        result += hexChar;
    }
    return result;
}
std::ostream& operator<<( std::ostream& stream, md5& context ) {
    stream << context.hex_digest();
    return stream;
}
void md5::init() {
    finalized = 0;  // we just started
    // nothing counted, so count = 0
    count[ 0 ] = 0;
    count[ 1 ] = 0;
    // load magic initialization constants.
    state[ 0 ] = 0x67452301;
    state[ 1 ] = 0xefcdab89;
    state[ 2 ] = 0x98badcfe;
    state[ 3 ] = 0x10325476;
}
// Constants for MD5Transform routine. Although we could use C++ style constants, defines are actually better, since
// they let us easily evade scope clashes.
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21
void md5::transform( uint1 block[ 64 ] ) {
    // MD5 basic transformation. Transforms state based on block.
    uint4 a = state[ 0 ], b = state[ 1 ], c = state[ 2 ], d = state[ 3 ], x[ 16 ];
    decode( x, block, 64 );
    assert( !finalized );  // not just a user error, since the method is private
    // Round 1
    FF( a, b, c, d, x[ 0 ], S11, 0xd76aa478 );   // 1
    FF( d, a, b, c, x[ 1 ], S12, 0xe8c7b756 );   // 2
    FF( c, d, a, b, x[ 2 ], S13, 0x242070db );   // 3
    FF( b, c, d, a, x[ 3 ], S14, 0xc1bdceee );   // 4
    FF( a, b, c, d, x[ 4 ], S11, 0xf57c0faf );   // 5
    FF( d, a, b, c, x[ 5 ], S12, 0x4787c62a );   // 6
    FF( c, d, a, b, x[ 6 ], S13, 0xa8304613 );   // 7
    FF( b, c, d, a, x[ 7 ], S14, 0xfd469501 );   // 8
    FF( a, b, c, d, x[ 8 ], S11, 0x698098d8 );   // 9
    FF( d, a, b, c, x[ 9 ], S12, 0x8b44f7af );   // 10
    FF( c, d, a, b, x[ 10 ], S13, 0xffff5bb1 );  // 11
    FF( b, c, d, a, x[ 11 ], S14, 0x895cd7be );  // 12
    FF( a, b, c, d, x[ 12 ], S11, 0x6b901122 );  // 13
    FF( d, a, b, c, x[ 13 ], S12, 0xfd987193 );  // 14
    FF( c, d, a, b, x[ 14 ], S13, 0xa679438e );  // 15
    FF( b, c, d, a, x[ 15 ], S14, 0x49b40821 );  // 16
    // Round 2
    GG( a, b, c, d, x[ 1 ], S21, 0xf61e2562 );   // 17
    GG( d, a, b, c, x[ 6 ], S22, 0xc040b340 );   // 18
    GG( c, d, a, b, x[ 11 ], S23, 0x265e5a51 );  // 19
    GG( b, c, d, a, x[ 0 ], S24, 0xe9b6c7aa );   // 20
    GG( a, b, c, d, x[ 5 ], S21, 0xd62f105d );   // 21
    GG( d, a, b, c, x[ 10 ], S22, 0x2441453 );   // 22
    GG( c, d, a, b, x[ 15 ], S23, 0xd8a1e681 );  // 23
    GG( b, c, d, a, x[ 4 ], S24, 0xe7d3fbc8 );   // 24
    GG( a, b, c, d, x[ 9 ], S21, 0x21e1cde6 );   // 25
    GG( d, a, b, c, x[ 14 ], S22, 0xc33707d6 );  // 26
    GG( c, d, a, b, x[ 3 ], S23, 0xf4d50d87 );   // 27
    GG( b, c, d, a, x[ 8 ], S24, 0x455a14ed );   // 28
    GG( a, b, c, d, x[ 13 ], S21, 0xa9e3e905 );  // 29
    GG( d, a, b, c, x[ 2 ], S22, 0xfcefa3f8 );   // 30
    GG( c, d, a, b, x[ 7 ], S23, 0x676f02d9 );   // 31
    GG( b, c, d, a, x[ 12 ], S24, 0x8d2a4c8a );  // 32
    // Round 3
    HH( a, b, c, d, x[ 5 ], S31, 0xfffa3942 );   // 33
    HH( d, a, b, c, x[ 8 ], S32, 0x8771f681 );   // 34
    HH( c, d, a, b, x[ 11 ], S33, 0x6d9d6122 );  // 35
    HH( b, c, d, a, x[ 14 ], S34, 0xfde5380c );  // 36
    HH( a, b, c, d, x[ 1 ], S31, 0xa4beea44 );   // 37
    HH( d, a, b, c, x[ 4 ], S32, 0x4bdecfa9 );   // 38
    HH( c, d, a, b, x[ 7 ], S33, 0xf6bb4b60 );   // 39
    HH( b, c, d, a, x[ 10 ], S34, 0xbebfbc70 );  // 40
    HH( a, b, c, d, x[ 13 ], S31, 0x289b7ec6 );  // 41
    HH( d, a, b, c, x[ 0 ], S32, 0xeaa127fa );   // 42
    HH( c, d, a, b, x[ 3 ], S33, 0xd4ef3085 );   // 43
    HH( b, c, d, a, x[ 6 ], S34, 0x4881d05 );    // 44
    HH( a, b, c, d, x[ 9 ], S31, 0xd9d4d039 );   // 45
    HH( d, a, b, c, x[ 12 ], S32, 0xe6db99e5 );  // 46
    HH( c, d, a, b, x[ 15 ], S33, 0x1fa27cf8 );  // 47
    HH( b, c, d, a, x[ 2 ], S34, 0xc4ac5665 );   // 48
    // Round 4
    II( a, b, c, d, x[ 0 ], S41, 0xf4292244 );   // 49
    II( d, a, b, c, x[ 7 ], S42, 0x432aff97 );   // 50
    II( c, d, a, b, x[ 14 ], S43, 0xab9423a7 );  // 51
    II( b, c, d, a, x[ 5 ], S44, 0xfc93a039 );   // 52
    II( a, b, c, d, x[ 12 ], S41, 0x655b59c3 );  // 53
    II( d, a, b, c, x[ 3 ], S42, 0x8f0ccc92 );   // 54
    II( c, d, a, b, x[ 10 ], S43, 0xffeff47d );  // 55
    II( b, c, d, a, x[ 1 ], S44, 0x85845dd1 );   // 56
    II( a, b, c, d, x[ 8 ], S41, 0x6fa87e4f );   // 57
    II( d, a, b, c, x[ 15 ], S42, 0xfe2ce6e0 );  // 58
    II( c, d, a, b, x[ 6 ], S43, 0xa3014314 );   // 59
    II( b, c, d, a, x[ 13 ], S44, 0x4e0811a1 );  // 60
    II( a, b, c, d, x[ 4 ], S41, 0xf7537e82 );   // 61
    II( d, a, b, c, x[ 11 ], S42, 0xbd3af235 );  // 62
    II( c, d, a, b, x[ 2 ], S43, 0x2ad7d2bb );   // 63
    II( b, c, d, a, x[ 9 ], S44, 0xeb86d391 );   // 64
    //
    state[ 0 ] += a;
    state[ 1 ] += b;
    state[ 2 ] += c;
    state[ 3 ] += d;
    memset( (uint1*) x, 0, sizeof( x ) );  // zeroize sensitive information
}
void md5::encode( uint1* output, uint4* input, uint4 len ) {
    // Encodes input (UINT4) into output (unsigned char). Assumes len is a multiple of 4.
    unsigned int i, j;
    for( i = 0, j = 0; j < len; i++, j += 4 ) {
        output[ j ] = ( uint1 )( input[ i ] & 0xff );
        output[ j + 1 ] = ( uint1 )( ( input[ i ] >> 8 ) & 0xff );
        output[ j + 2 ] = ( uint1 )( ( input[ i ] >> 16 ) & 0xff );
        output[ j + 3 ] = ( uint1 )( ( input[ i ] >> 24 ) & 0xff );
    }
}
void md5::decode( uint4* output, uint1* input, uint4 len ) {
    // Decodes input (unsigned char) into output (UINT4). Assumes len is a multiple of 4.
    unsigned int i, j;
    for( i = 0, j = 0; j < len; i++, j += 4 )
        output[ i ] = ( (uint4) input[ j ] ) | ( ( (uint4) input[ j + 1 ] ) << 8 ) |
                      ( ( (uint4) input[ j + 2 ] ) << 16 ) | ( ( (uint4) input[ j + 3 ] ) << 24 );
}
void md5::memcpy( uint1* output, uint1* input, uint4 len ) {
    // Note: Replace "for loop" with standard memcpy if possible.
    unsigned int i;
    for( i = 0; i < len; i++ )
        output[ i ] = input[ i ];
}
void md5::memset( uint1* output, uint1 value, uint4 len ) {
    // Note: Replace "for loop" with standard memset if possible.
    unsigned int i;
    for( i = 0; i < len; i++ )
        output[ i ] = value;
}
inline unsigned int md5::rotate_left( uint4 x, uint4 n ) {
    return ( x << n ) | ( x >> ( 32 - n ) );
}  // ROTATE_LEFT rotates x left n bits.
inline unsigned int md5::F( uint4 x, uint4 y, uint4 z ) {
    return ( x & y ) | ( ~x & z );
}  // F, G, H and I are basic MD5 functions.
inline unsigned int md5::G( uint4 x, uint4 y, uint4 z ) {
    return ( x & z ) | ( y & ~z );
}
inline unsigned int md5::H( uint4 x, uint4 y, uint4 z ) {
    return x ^ y ^ z;
}
inline unsigned int md5::I( uint4 x, uint4 y, uint4 z ) {
    return y ^ ( x | ~z );
}
// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4. Rotation is separate from addition to prevent
// recomputation.
inline void md5::FF( uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac ) {
    a += F( b, c, d ) + x + ac;
    a = rotate_left( a, s ) + b;
}
inline void md5::GG( uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac ) {
    a += G( b, c, d ) + x + ac;
    a = rotate_left( a, s ) + b;
}
inline void md5::HH( uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac ) {
    a += H( b, c, d ) + x + ac;
    a = rotate_left( a, s ) + b;
}
inline void md5::II( uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac ) {
    a += I( b, c, d ) + x + ac;
    a = rotate_left( a, s ) + b;
}

};  // namespace tools

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace signal {

std::atomic_bool g_bStop{ false };

bool get_signal_description( int nSignalNo,
                             std::string& strSignalName,
                             std::string& strSignalDescription ) {  // returns true if signal name is known
    struct sig_desc_t {
        int nSignalNo;
        const char* strSignalName;
        const char* strSignalDescription;
    };  /// struct sig_desc_t
    static const sig_desc_t g_arr[] = {
        { SIGINT,
          "SIGINT",
          "This signal is the same as pressing ctrl-c. On some systems, \"delete\" + \"break\" sends the same signal "
          "to the process. The process is interrupted and stopped. However, the process can ignore this signal." },
        { SIGTERM,
          "SIGTERM",
          "Requests a process to stop running. This signal can be ignored. The process is given time to gracefully "
          "shutdown. When a program gracefully shuts down, that means it is given time to save its progress and "
          "release resources. In other words, it is not forced to stop. SIGINT is very similar to SIGTERM." },
        { SIGKILL,
          "SIGKILL",
          "Forces the process to stop executing immediately. The program cannot ignore this signal. This process does "
          "not get to clean-up either." },
        { SIGHUP,
          "SIGHUP",
          "Disconnects a process from the parent process. This an also be used to restart processes. For example, "
          "\"killall -SIGUP compiz\" will restart Compiz. This is useful for daemons with memory leaks." },
        { SIGQUIT,
          "SIGQUIT",
          "The SIGQUIT sigma; is like SIGINT with the ability to make the process produce a core dump." },
        { SIGILL,
          "SIGILL",
          "When a process performs a faulty, forbidden, or unknown function, the system sends the SIGILL signal to the "
          "process. This is the ILLegal SIGnal." },
        { SIGABRT,
          "SIGABRT",
          "This is the abort signal. Typically, a process will initiate this kill signal on itself." },
        { SIGSTOP,
          "SIGSTOP",
          "This signal makes the operating system pause a process's execution. The process cannot ignore the signal." },
        { SIGFPE,
          "SIGFPE",
          "Processes that divide by zero are killed using SIGFPE. Imagine if humans got the death penalty for such "
          "math. NOTE: The author of this article was recently drug out to the street and shot for dividing by zero." },
        { SIGSEGV, "SIGSEGV", "When an application has a segmentation violation, this signal is sent to the process." },
        { SIGTSTP,
          "SIGTSTP",
          "This signal is like pressing ctrl-z. This makes a request to the terminal containing the process to ask the "
          "process to stop temporarily. The process can ignore the request." },
        { SIGCONT,
          "SIGCONT",
          "To make processes continue executing after being paused by the SIGTSTP or SIGSTOP signal, send the SIGCONT "
          "signal to the paused process. This is the CONTinue SIGnal. This signal is beneficial to Unix job control "
          "(executing background tasks)." },
        { SIGALRM, "SIGALRM", "Sent when the real time or clock time timer expires." },
        { SIGTRAP,
          "SIGTRAP",
          "This signal is used for debugging purposes. When a process has performed an action or a condition is met "
          "that a debugger is waiting for, this signal will be sent to the process." },
        { SIGBUS,
          "SIGBUS",
          "When a process is sent the SIGBUS signal, it is because the process caused a bus error. Commonly, these bus "
          "errors are due to a process trying to use fake physical addresses or the process has its memory alignment "
          "set incorrectly." },
        { SIGUSR1,
          "SIGUSR1",
          "This indicates a user-defined condition. This signal can be set by the user by programming the commands in "
          "sigusr1.c. This requires the programmer to know C/C++." },
        { SIGUSR2, "SIGUSR2", "This indicates a user-defined condition." },
        { SIGPIPE,
          "SIGPIPE",
          "When a process tries to write to a pipe that lacks an end connected to a reader, this signal is sent to the "
          "process. A reader is a process that reads data at the end of a pipe." },
        { SIGCHLD,
          "SIGCHLD",
          "When a parent process loses its child process, the parent process is sent the SIGCHLD signal. This cleans "
          "up resources used by the child process. In computers, a child process is a process started by another "
          "process know as a parent." },
        { SIGTTIN,
          "SIGTTIN",
          "When a process attempts to read from a tty (computer terminal), the process receives this signal." },
        { SIGTTOU,
          "SIGTTOU",
          "When a process attempts to write from a tty (computer terminal), the process receives this signal." },
        { SIGURG,
          "SIGURG",
          "When a process has urgent data to be read or the data is very large, the SIGURG signal is sent to the "
          "process." },
        { SIGXCPU,
          "SIGXCPU",
          "When a process uses the CPU past the allotted time, the system sends the process this signal. SIGXCPU acts "
          "like a warning; the process has time to save the progress (if possible) and close before the system kills "
          "the process with SIGKILL." },
        { SIGXFSZ,
          "SIGXFSZ",
          "Filesystems have a limit to how large a file can be made. When a program tries to violate this limit, the "
          "system will send that process the SIGXFSZ signal." },
        { SIGVTALRM, "SIGVTALRM", "Sent when CPU time used by the process elapses." },
        { SIGPROF,
          "SIGPROF",
          "Sent when CPU time used by the process and by the system on behalf of the process elapses." },
        { SIGWINCH,
          "SIGWINCH",
          "When a process is in a terminal that changes its size, the process receives this signal." },
        { SIGIO, "SIGIO", "Alias to SIGPOLL or at least behaves much like SIGPOLL." },
#if( defined SIGPWR )
        { SIGPWR,
          "SIGPWR",
          "Power failures will cause the system to send this signal to processes (if the system is still on)." },
#endif
        { SIGSYS, "SIGSYS", "Processes that give a system call an invalid parameter will receive this signal." },
#if( defined SIGEMT )
        { SIGEMT, "SIGEMT", "Processes receive this signal when an emulator trap occurs." },
#endif
#if( defined SIGINFO )
        { SIGINFO,
          "SIGINFO",
          "Terminals may sometimes send status requests to processes. When this happens, processes will also receive "
          "this signal." },
#endif
#if( defined SIGLOST )
        { SIGLOST, "SIGLOST", "Processes trying to access locked files will get this signal." },
#endif
#if( defined SIGPOLL )
        { SIGPOLL, "", "When a process causes an asynchronous I/O event, that process is sent the SIGPOLL signal." },
#endif
    };
    for( size_t i = 0; i < ( sizeof( g_arr ) / sizeof( g_arr[ 0 ] ) ); ++i ) {
        const sig_desc_t& desc = g_arr[ i ];
        if( desc.nSignalNo == nSignalNo ) {
            strSignalName = desc.strSignalName;
            strSignalDescription = desc.strSignalDescription;
            return true;
        }
    }
    {  // blk
        std::stringstream ss;
        ss << nSignalNo;
        strSignalName = ss.str();
    }  // blk
#if( defined SIGRTMIN ) && ( defined SIGRTMAX )
    int nRangeMin = SIGRTMIN, nRangeMax = SIGRTMAX;
    if( nRangeMin <= nSignalNo && nSignalNo <= nRangeMax ) {
        int nDistFromMin = nSignalNo - nRangeMin;
        std::stringstream ss;
        ss << "Signal " << nSignalNo << " is in range [SIGRTMIN(" << nRangeMin << ")...SIGRTMAX(" << nRangeMax << ")]."
           << " Distance from SIGRTMIN is " << nDistFromMin << "."
           << "This is a set of signals that varies between systems."
           << " They are labeled SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3, ......., and so on (usually up to 15)."
           << " These are user-defined signals; they must be programmed in the Linux kernel's source code."
           << " That would require the user to know C/C++.";
        strSignalDescription = ss.str();
        return false;
    }
#endif  /// (defined SIGRTMIN) && (defined SIGRTMAX)
    strSignalDescription = "Unknown signal type.";
    return false;
}

std::string signal2str( int nSignalNo, const char* strPrefix, const char* strSuffix, bool isAddDesc ) {
    std::string strSignalName, strSignalDescription;
    bool bHaveSignalName = get_signal_description( nSignalNo, strSignalName, strSignalDescription );
    std::stringstream ss;
    ss << ( ( strPrefix == nullptr ) ? "Signal " : strPrefix );
    if( bHaveSignalName )
        ss << strSignalName << "(" << nSignalNo << ")";
    else
        ss << nSignalNo;
    if( isAddDesc )
        ss << ". " << strSignalDescription;
    if( strSuffix != nullptr )
        ss << strSuffix;
    return ss.str();
}

bool init_common_signal_handling( fn_signal_handler_t fnSignalHander ) {
    if( fnSignalHander == nullptr )
        return false;
    struct sigaction sa;
    sa.sa_handler = fnSignalHander;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;
    //
    // Handled:
    // The SIGINT signal is the same as pressing ctrl-c. On some systems, "delete" + "break" sends the same signal to
    // the process. The process is interrupted and stopped. However, the process can ignore this signal.
    sigaction( SIGINT, &sa, NULL );
    // SIGTERM - This signal requests a process to stop running. This signal can be ignored. The process is given time
    // to gracefully shutdown. When a program gracefully shuts down, that means it is given time to save its progress
    // and release resources. In other words, it is not forced to stop. SIGINT is very similar to SIGTERM.
    sigaction( SIGTERM, &sa, NULL );
    // SIGKILL - The SIGKILL signal forces the process to stop executing immediately. The program cannot ignore this
    // signal. This process does not get to clean-up either.
    sigaction( SIGKILL, &sa, NULL );
    // The SIGHUP signal disconnects a process from the parent process. This an also be used to restart processes. For
    // example, "killall -SIGUP compiz" will restart Compiz. This is useful for daemons with memory leaks.
    sigaction( SIGHUP, &sa, NULL );
    // The SIGQUIT sigma; is like SIGINT with the ability to make the process produce a core dump.
    sigaction( SIGQUIT, &sa, NULL );
    // SIGILL - When a process performs a faulty, forbidden, or unknown function, the system sends the SIGILL signal to
    // the process. This is the ILLegal SIGnal.
    sigaction( SIGILL, &sa, NULL );
    // The SIGABRT signal is the abort signal. Typically, a process will initiate this kill signal on itself.
    sigaction( SIGABRT, &sa, NULL );
    // SIGSTOP - This signal makes the operating system pause a process's execution. The process cannot ignore the
    // signal.
    sigaction( SIGSTOP, &sa, NULL );
    // SIGFPE - Processes that divide by zero are killed using SIGFPE. Imagine if humans got the death penalty for such
    // math. NOTE: The author of this article was recently drug out to the street and shot for dividing by zero.
    sigaction( SIGFPE, &sa, NULL );
    // SIGPIPE - When a process tries to write to a pipe that lacks an end connected to a reader, this signal is sent to
    // the process. A reader is a process that reads data at the end of a pipe.
    sigaction( SIGPIPE, &sa, NULL );
    //
    // Not handled:
    // SIGSEGV - When an application has a segmentation violation, this signal is sent to the process.
    // SIGTSTP - This signal is like pressing ctrl-z. This makes a request to the terminal containing the process to ask
    // the process to stop temporarily. The process can ignore the request. SIGCONT - To make processes continue
    // executing after being paused by the SIGTSTP or SIGSTOP signal, send the SIGCONT signal to the paused process.
    // This is the CONTinue SIGnal. This signal is beneficial to Unix job control (executing background tasks). SIGALRM
    // - SIGALRM is sent when the real time or clock time timer expires. SIGTRAP - This signal is used for debugging
    // purposes. When a process has performed an action or a condition is met that a debugger is waiting for, this
    // signal will be sent to the process. SIGBUS  - When a process is sent the SIGBUS signal, it is because the process
    // caused a bus error. Commonly, these bus errors are due to a process trying to use fake physical addresses or the
    // process has its memory alignment set incorrectly. SIGUSR1 - This indicates a user-defined condition. This signal
    // can be set by the user by programming the commands in sigusr1.c. This requires the programmer to know C/C++.
    // SIGUSR2 - This indicates a user-defined condition.
    // SIGPIPE - When a process tries to write to a pipe that lacks an end connected to a reader, this signal is sent to
    // the process. A reader is a process that reads data at the end of a pipe. SIGCHLD - When a parent process loses
    // its child process, the parent process is sent the SIGCHLD signal. This cleans up resources used by the child
    // process. In computers, a child process is a process started by another process know as a parent. SIGTTIN - When a
    // process attempts to read from a tty (computer terminal), the process receives this signal. SIGTTOU - When a
    // process attempts to write from a tty (computer terminal), the process receives this signal. SIGURG  - When a
    // process has urgent data to be read or the data is very large, the SIGURG signal is sent to the process. SIGXCPU -
    // When a process uses the CPU past the allotted time, the system sends the process this signal. SIGXCPU acts like a
    // warning; the process has time to save the progress (if possible) and close before the system kills the process
    // with SIGKILL. SIGXFSZ - Filesystems have a limit to how large a file can be made. When a program tries to violate
    // this limit, the system will send that process the SIGXFSZ signal. SIGVTALRM - SIGVTALRM is sent when CPU time
    // used by the process elapses. SIGPROF - SIGPROF is sent when CPU time used by the process and by the system on
    // behalf of the process elapses. SIGWINCH - When a process is in a terminal that changes its size, the process
    // receives this signal. SIGIO   - Alias to SIGPOLL or at least behaves much like SIGPOLL. SIGPWR  - Power failures
    // will cause the system to send this signal to processes (if the system is still on). SIGSYS  - Processes that give
    // a system call an invalid parameter will receive this signal. SIGRTMIN* - This is a set of signals that varies
    // between systems. They are labeled SIGRTMIN+1, SIGRTMIN+2, SIGRTMIN+3, ......., and so on (usually up to 15).
    // These are user-defined signals; they must be programmed in the Linux kernel's source code. That would require the
    // user to know C/C++. SIGRTMAX* - This is a set of signals that varies between systems. They are labeled
    // SIGRTMAX-1, SIGRTMAX-2, SIGRTMAX-3, ......., and so on (usually up to 14). These are user-defined signals; they
    // must be programmed in the Linux kernel's source code. That would require the user to know C/C++. SIGEMT  -
    // Processes receive this signal when an emulator trap occurs. SIGINFO - Terminals may sometimes send status
    // requests to processes. When this happens, processes will also receive this signal. SIGLOST - Processes trying to
    // access locked files will get this signal. SIGPOLL - When a process causes an asynchronous I/O event, that process
    // is sent the SIGPOLL signal.
    //
    ::signal( SIGPIPE, SIG_IGN );
    //
    return true;
}

void print_backtrace() {
#ifdef __HUB_BUILDING_TARGET_ARCH_MIPS__
    ::fprintf( stderr, "\nMIPS toolchain doesn't support backtrace functionality!\n" );
#elif _LIBCPP_HAS_MUSL_LIBC
    ::fprintf( stderr, "\nToolchain based on musl doesn't support backtrace functionality!\n" );
#else
    std::cout.flush();
    std::cerr.flush();
    void* arrBacktrace[ 30 ];
    int cntBacktrace = ::backtrace(
        arrBacktrace,
        sizeof( arrBacktrace ) / sizeof( arrBacktrace[ 0 ] ) );  // get void*'s for all entries on the stack
    // print out all the frames to stderr
    ::fprintf( stderr, "\n\nbacktrace of %d symbol(s):\n", int( cntBacktrace ) );
    ::fflush( stderr );
    ::backtrace_symbols_fd( arrBacktrace, cntBacktrace, STDERR_FILENO );
    ::fflush( stderr );
    ::fprintf( stderr, "\n" );
    ::fflush( stderr );
    std::cout.flush();
    std::cerr.flush();
#endif  // __HUB_BUILDING_TARGET_ARCH_MIPS__
}
};  // namespace signal

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

};  // namespace tiny
