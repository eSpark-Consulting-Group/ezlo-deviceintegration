#pragma once

#include <list>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <chrono>
#include <atomic>

#include "json_utils/json_wrapper.h"

namespace ezlo {

namespace common {

namespace APIs {
extern void json_set_time_mark( const std::chrono::high_resolution_clock::time_point& ptTime,
                                JsonObject& jo,
                                const char* strAttrNameLocal = "timestamp",    // if nullptr, do not add attribute
                                const char* strAttrNameUTC = "timestamp_utc",  // if nullptr, do not add attribute
                                bool isDaysInsteadOfYMD = false );
extern void json_set_now_mark( JsonObject& jo,
                               const char* strAttrNameLocal = "timestamp",    // if nullptr, do not add attribute
                               const char* strAttrNameUTC = "timestamp_utc",  // if nullptr, do not add attribute
                               bool isDaysInsteadOfYMD = false );

extern bool extract_set_of_strings_from_json_array( const JsonArray& jarr, std::set< std::string >& a_set );
extern bool extract_set_of_strings_from_json_parms( const char* strNameArray,
                                                    const char* strNameSingle,
                                                    const JsonObject& params,
                                                    std::set< std::string >& a_set,
                                                    bool bNotPresentPropsOKay = true );

extern bool file2string( const char* strFileName, std::string& s );
extern std::string file2string( const char* strFileName );
extern bool string2file( const char* strFileName, const char* s );
extern bool string2file( const char* strFileName, const std::string& s );
extern bool file2json( const char* strFileName, JsonObject& jo );
extern bool json2file( const char* strFileName, const JsonObject& jo );
extern std::vector< unsigned char > file2vec( const char* strFileName );
extern std::string file2base64( const char* strFileName );
extern std::string base64Encode( const std::vector< unsigned char >& vec );
extern std::string base64Decode( const std::string& str );

extern std::string encode_command_line_argument( const char* strArg );
extern std::string decode_command_line_argument( const char* strArg );
inline std::string encode_command_line_argument( const std::string& strArg ) {
    return encode_command_line_argument( strArg.c_str() );
}
inline std::string decode_command_line_argument( const std::string& strArg ) {
    return decode_command_line_argument( strArg.c_str() );
}

extern std::list< std::string > split_string( const char* text, const char* delims );
extern std::list< std::string > split_string( const std::string& text, const std::string& delims );

template < typename E >
constexpr auto EnumToIntegral( E e ) -> typename std::underlying_type< E >::type {
    return static_cast< typename std::underlying_type< E >::type >( e );
}

};  // namespace APIs

};  // namespace common

};  // namespace ezlo
