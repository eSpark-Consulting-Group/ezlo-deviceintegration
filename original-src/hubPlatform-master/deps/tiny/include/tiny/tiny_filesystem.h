#if( !defined __TINY_FILESYSTEM_H )
#define __TINY_FILESYSTEM_H 1

/// l_sergiy: NodeJS style FileSystem class based on https://github.com/hxoht/nodeuv-fs and
/// https://github.com/hxoht/nodeuv-buffer

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>

extern "C" {
#include "uv.h"
};  /// extern "C"

namespace tiny {

class buffer_object {
public:
    uv_buf_t data;
    std::string toString() const;
    int length() const;
    int copy( const buffer_object& b );
    int copy( const buffer_object& b, int target_start );
    int copy( const buffer_object& b, int target_start, int source_start );
    int copy( const buffer_object& b, int target_start, int source_start, int source_end );
    buffer_object();
    buffer_object( int size );
    buffer_object( char* str );
    buffer_object( const std::string& str );
    buffer_object( const buffer_object& buf );
    buffer_object& operator=( const buffer_object& buf );
};  /// class buffer_object

// https://github.com/iojs/io.js/blob/master/lib/fs.js#L256
const unsigned int MAGIC_BUFFER_SIZE = 8192;

struct stats_object {
    uint64_t dev;
    uint64_t mode;
    uint64_t nlink;
    uint64_t uid;
    uint64_t gid;
    uint64_t rdev;
    uint64_t ino;
    uint64_t size;
    uint64_t blksize;
    uint64_t blocks;
    uint64_t flags;
    uint64_t gen;
    uv_timespec_t atime;
    uv_timespec_t mtime;
    uv_timespec_t ctime;
    uv_timespec_t birthtime;
};  /// struct stats_object

template < typename... Args >
using Callback = std::function< void( Args... params ) >;

class error_object {
public:
    std::string message = "";
    int code = 0;
    inline operator bool() const {
        return ( message.length() > 0 || code != 0 );
    }
};  /// class error_object

struct read_options {  // some reasonable defaults for Read and Write options...
    int flags = O_CREAT | O_RDWR;
    int mode = S_IRUSR | S_IWUSR;
};  /// struct read_options

struct write_options {
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
};  /// struct write_options

class filesystem {
public:
    uv_loop_t* UV_LOOP;
    bool running = false;

    std::string cwd();

    void stat( const std::string&, Callback< error_object, stats_object > );
    stats_object statSync( const std::string& );

    error_object mkdirSync( const std::string& );
    error_object mkdirSync( const std::string&, int );

    error_object rmdirSync( const std::string& );

    void open( const std::string&, int, int, Callback< error_object, uv_file > );
    int openSync( const std::string&, int, int );

    void read( uv_file, int64_t, int64_t, Callback< error_object, uv_buf_t > );
    int readSync( uv_file fd, uv_buf_t* uvb, int64_t offset, int64_t bytes );

    void write( uv_file, const uv_buf_t&, int64_t, Callback< error_object > );
    int writeSync( uv_file fd, const buffer_object& buf, int64_t offset, int64_t length );

    void close( uv_file fd, Callback< error_object > );
    int closeSync( uv_file fd );

    // higher level functions
    void readFile( const std::string&, Callback< error_object, buffer_object > );
    void readFile( const std::string&, const read_options&, Callback< error_object, buffer_object > );

    buffer_object readFileSync( const std::string&, const read_options& );
    buffer_object readFileSync( const std::string& );

    void writeFile( const std::string&, const std::string&, Callback< error_object > );
    void writeFile( const std::string&, const std::string&, const write_options&, Callback< error_object > );
    void writeFile( const std::string&, const uv_buf_t&, Callback< error_object > );
    void writeFile( const std::string&, const uv_buf_t&, const write_options&, Callback< error_object > );
    int writeFileSync( const std::string&, const buffer_object& );
    int writeFileSync( const std::string&, const buffer_object&, const write_options& );

    filesystem() {
        UV_LOOP = uv_default_loop();
    }
};  /// class filesystem

};  // namespace tiny

#endif  /// (!defined __TINY_FILESYSTEM_H)
