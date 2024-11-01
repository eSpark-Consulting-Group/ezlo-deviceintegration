// note: process controller is based on https://github.com/eidheim/tiny-process-library

#include <tiny/tiny_process.h>
#include <tiny/tiny_cxx_replacements.h>
#include <tiny/tiny_multithreading.h>
#include <tiny/tiny_utils.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if( defined _WIN32 )
#include <windows.h>
#include <cstring>
#include <TlHelp32.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <libgen.h>
#endif  // (defined _WIN32)

#if( defined __BUILDING_4_MAC_OS_X__ )
#include <mach-o/dyld.h>
#endif  // (defined __BUILDING_4_MAC_OS_X__)

// note: if following is uncommented, then do debug messages output to standard error stream
//#define __EZLO_DEBUG_LOGGING_PROCESS_APIS__ 1
// note: if following is uncommented, then do debug messages output to standard error stream
//#define __EZLO_DEBUG_LOGGING_WATCHER_APIS__ 1

#include <sys/types.h>
#include <sys/stat.h>

namespace tiny {

namespace tools {

#if( !defined _WIN32 )
std::string calc_path_of_current_exe() {
#if( defined __BUILDING_4_MAC_OS_X__ )
    char path[ 2048 ];
    uint32_t size = sizeof( path );
    if( _NSGetExecutablePath( path, &size ) == 0 )
        printf( "executable path is %s\n", path );
    else
        throw std::runtime_error( "internal error of calc_path_of_current_exe(), buffer to small" );
    return path;
#else   // (defined __BUILDING_4_MAC_OS_X__)
        // based on https://stackoverflow.com/questions/23943239/how-to-get-path-to-current-exe-file-on-linux
    char strResult[ 512 ];
    ::memset( strResult, 0, sizeof( strResult ) );
    ssize_t cnt = ::readlink( "/proc/self/exe", strResult, sizeof( strResult ) / sizeof( strResult[ 0 ] ) - 1 );
    std::string strPath;
    if( cnt != -1 )
        strPath = strResult;
    return strPath;
#endif  // else from (defined __BUILDING_4_MAC_OS_X__)
}
std::string calc_dir_of_current_exe() {  // without '/' at the end of string, empty if fail
    std::string strPath = calc_path_of_current_exe();
    if( !strPath.empty() )
        strPath = ::dirname( const_cast< char* >( strPath.c_str() ) );
    return strPath;
}
#endif  // (!defined _WIN32)

std::vector< unsigned char > file2vec( const char* strFileName ) {
    std::vector< unsigned char > vec;
    if( strFileName == nullptr || strFileName[ 0 ] == '\0' )
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
extern std::string file2string3vec( const char* strFileName ) {
    std::vector< unsigned char > vec = file2vec( strFileName );
    std::string s;
    std::vector< unsigned char >::const_iterator itWalk = vec.begin(), itEnd = vec.end();
    for( ; itWalk != itEnd; ++itWalk ) {
        s += char( *itWalk );
    }
    return s;
}
bool file2stringNoSize( const char* strFileName, std::string& s, int nLimitLineCount ) {
    s.clear();
    if( strFileName == nullptr || strFileName[ 0 ] == '\0' )
        return false;
    FILE* f = ::fopen( strFileName, "r" );
    if( !f )
        return false;
    size_t z = 0;
    char* strDynLine = 0;
    int nLineNo = 0;
    do {
        if( nLimitLineCount >= 0 && nLineNo >= nLimitLineCount )
            break;
        ssize_t nLoaded = ::getline( &strDynLine, &z, f );
        if( nLoaded < 0 )
            break;
        // handle strDynLine line of nLoaded count of chars
        for( ssize_t i = 0; i < nLoaded; ++i )
            s += strDynLine[ i ];
        ::free( strDynLine );
        ++nLineNo;
    } while( !::feof( f ) );
    ::fclose( f );
    return true;
}
std::string file2stringNoSize( const char* strFileName, int nLimitLineCount ) {
    std::string s;
    if( !file2stringNoSize( strFileName, s, nLimitLineCount ) )
        s.clear();
    return s;
}
bool file2string( const char* strFileName, std::string& s ) {
    s.clear();
    if( strFileName == nullptr || strFileName[ 0 ] == '\0' )
        return false;
    try {
        std::ifstream t( strFileName, std::ios_base::binary );
        t.imbue( std::locale( "C" ) );
        t.seekg( 0, std::ios::end );
        s.reserve( size_t( t.tellg() ) );
        t.seekg( 0, std::ios::beg );
        s.assign( ( std::istreambuf_iterator< char >( t ) ), std::istreambuf_iterator< char >() );
        return true;
    }
    catch( std::exception& ) {
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
    if( strFileName == nullptr || strFileName[ 0 ] == '\0' )
        return false;
    try {
        size_t len = ( s != nullptr && s[ 0 ] != '\0' ) ? ::strlen( s ) : 0;
        std::ofstream t( strFileName, std::ios_base::binary );
        t.imbue( std::locale( "C" ) );
        if( len > 0 )
            t.write( s, std::streamsize( len ) );
        t.close();
        return true;
    }
    catch( std::exception& ) {
        return false;
    }
    catch( ... ) {
        throw;
    }
}
bool string2file( const char* strFileName, const std::string& s ) {
    return string2file( strFileName, s.empty() ? "" : s.c_str() );
}
string_list_t splitCmdLine( const char* strCmdLine ) {
    return split( strCmdLine ? strCmdLine : "", ' ' );
}
std::string findCmdLineArgValue( string_list_t lstCmdLine, const char* strArg ) {
    std::string s;
    if( strArg == nullptr || strArg[ 0 ] == '\0' || lstCmdLine.size() < 2 )
        return s;
    string_list_t::const_iterator itWalk = lstCmdLine.begin(), itEnd = lstCmdLine.end();
    for( ++itWalk; itWalk != itEnd; ++itWalk ) {
        string_list_t::const_reference wlk = ( *itWalk );
        if( wlk == strArg ) {
            ++itWalk;
            if( itWalk == itEnd )
                break;
            s = ( *itWalk );
            break;
        }
    }
    return s;
}

//#if (!defined _WIN32)
extern std::string get_process_dir_entry_path( pid_type_t a_pid ) {
    std::ostringstream ss;
    ss << "/proc/" << (unsigned) a_pid;
    return ss.str();
}
std::string get_process_cmd_line( pid_type_t a_pid, const char* strSep ) {
    std::string cmdLine, strDirEntry = get_process_dir_entry_path( a_pid ) + "/cmdline";
    FILE* f = ::fopen( strDirEntry.c_str(), "rb" );
    if( f == nullptr )
        return cmdLine;
    char* arg = nullptr;
    size_t size = 0;
    while(::getdelim( &arg, &size, 0, f ) != -1 ) {
        if( strSep != nullptr && strSep[ 0 ] != '\0' && ( !cmdLine.empty() ) )
            cmdLine += strSep;
        cmdLine += arg;
    }
    ::free( arg );
    ::fclose( f );
    return rtrim_copy( cmdLine );
}
void replace_substring( std::string& str,
                        const std::string& strWhat,
                        const std::string& strWith,
                        size_t nMaxReplacements = std::string::npos ) {
    size_t index = 0, done = 0, lenWhat = strWhat.length(), lenWith = strWith.length();
    while( true ) {
        if( nMaxReplacements != std::string::npos && done >= nMaxReplacements )
            break;
        index = str.find( strWhat, index );  // locate the substring to replace
        if( index == std::string::npos )
            break;
        ++done;
        str.replace( index, lenWhat, strWith );  // make the replacement
        index += lenWith;  // advance index forward so the next iteration doesn't pick it up as well.
    }
}
process_info_list_t find_processes_by_name( const char* strExecutableName ) {  // nullptr name to get all (NOT FINISHED)
    process_info_list_t lst;
    string_list_t lstDirEntries = tiny::tools::enum_dir( "/proc", true, false );
    string_list_t::const_iterator itWalk = lstDirEntries.begin(), itEnd = lstDirEntries.end();
    for( ; itWalk != itEnd; ++itWalk ) {
        string_list_t::const_reference strDirEntry = ( *itWalk );
        std::string strPath;
        strPath = "/proc/";
        strPath += strDirEntry;
        strPath += "/comm";
        std::string strWalkExecutableName = rtrim_copy( file2stringNoSize( strPath.c_str() ) );
        if( strWalkExecutableName.empty() )
            continue;
        if( strExecutableName == nullptr || strExecutableName[ 0 ] == '\0' ||
            strWalkExecutableName == strExecutableName ) {
            process_info_list_t::value_type nfo;
            nfo.pid_ = ( pid_type_t )::atol( strDirEntry.c_str() );
            nfo.cmdLine_ = get_process_cmd_line( nfo.pid_, " " );
            lst.push_back( nfo );
            continue;
        }
    }  // for( ; itWalk != itEnd; ++itWalk )
    return lst;
}
process_info_list_t find_processes_by_cmd_line( const char* strCmdLine ) {
    process_info_list_t lst;
    if( strCmdLine == nullptr || strCmdLine[ 0 ] == '\0' )
        return lst;
    process_info_list_t lstAll = find_processes_by_name();
    if( lstAll.empty() )
        return lst;
    // std::cout << "---searching process --> " << strCmdLine << "\n";
    process_info_list_t::const_iterator itWalk = lstAll.begin(), itEnd = lstAll.end();
    for( ; itWalk != itEnd; ++itWalk ) {
        process_info_list_t::const_reference pi = ( *itWalk );
        if( pi.cmdLine_.empty() )
            continue;
        std::string s( pi.cmdLine_ );
        replace_substring( s, "  ", " " );

        // std::cout << "---walk process(" << pi.pid_ << ") -> " << s << "\n";
        if( s == strCmdLine ) {
            // std::cout << "===found process(" << pi.pid_ << ") -> " << s << "\n";
            lst.push_back( pi );
        }
    }  // for( ; itWalk != itEnd; ++itWalk )
    return lst;
}
//#endif // (!defined _WIN32)

};  // namespace tools

#if( defined _WIN32 )
class Handle {  // simple HANDLE wrapper to close it automatically from the destructor
    HANDLE handle;

public:
    Handle()
    : handle( INVALID_HANDLE_VALUE ) {}
    ~Handle() {
        close();
    }
    void close() {
        if( handle != INVALID_HANDLE_VALUE )
            CloseHandle( handle );
    }
    HANDLE detach() {
        HANDLE old_handle = handle;
        handle = INVALID_HANDLE_VALUE;
        return old_handle;
    }
    operator HANDLE() const {
        return handle;
    }
    HANDLE* operator&() {
        return &handle;
    }
};  /// class Handle
tiny::multithreading::mutex_type create_process_mutex(
    "MTX-WIN32-CREATE-PROCESS" );  // based on the discussion thread:
                                   // https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxq1wsj
#endif  // (defined _WIN32)

#if( defined _WIN32 )
process::Data::Data()
: id( 0 )
, handle( nullptr ) {}
#else   // (defined _WIN32)
process::Data::Data()
: id( -1 ) {}
#endif  // else from (defined _WIN32)

process::process( const string_type_t& strCommand,
                  const string_type_t& strPath,
                  std::function< void( const char* bytes, size_t n ) > read_stdout,
                  std::function< void( const char* bytes, size_t n ) > read_stderr,
                  bool open_stdin,
                  size_t buffer_size )
: bWasClosed_( true )
, mutexClose_( "MTX-PROCESS-CLOSE" )
, read_stdout( read_stdout )
, read_stderr( read_stderr )
, open_stdin( open_stdin )
, stdin_mutex( "MTX-PROCESS-STDIN" )
, buffer_size( buffer_size ) {
    open( strCommand, strPath );
    async_read();
}
#if( !defined _WIN32 )
process::process( std::function< void() > function,
                  std::function< void( const char*, size_t ) > read_stdout,
                  std::function< void( const char*, size_t ) > read_stderr,
                  bool open_stdin,
                  size_t buffer_size )
: bWasClosed_( true )
, mutexClose_( "MTX-PROCESS-CLOSE" )
, read_stdout( read_stdout )
, read_stderr( read_stderr )
, open_stdin( open_stdin )
, stdin_mutex( "MTX-PROCESS-STDIN" )
, buffer_size( buffer_size ) {
    open( function );
    async_read();
}
#endif  // (!defined _WIN32)

process::~process() {
    close_fds();
}

pid_type_t process::get_process_id() {  // get the process id of the started process
    return data_.id;
}

bool process::is_closed() const {
    return bWasClosed_;
}

bool process::write( const std::string& a_data ) {
    return write( a_data.c_str(), a_data.size() );
}

#if( defined _WIN32 )
// based on the example at https://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx.
process::pid_type_t process::open( const string_type_t& strCommand, const string_type_t& strPath ) {
    if( open_stdin )
        stdin_fd = std::unique_ptr< fd_type_t >( new fd_type_t( nullptr ) );
    if( read_stdout )
        stdout_fd = std::unique_ptr< fd_type_t >( new fd_type_t( nullptr ) );
    if( read_stderr )
        stderr_fd = std::unique_ptr< fd_type_t >( new fd_type_t( nullptr ) );
    Handle stdin_rd_p;
    Handle stdin_wr_p;
    Handle stdout_rd_p;
    Handle stdout_wr_p;
    Handle stderr_rd_p;
    Handle stderr_wr_p;
    SECURITY_ATTRIBUTES security_attributes;
    security_attributes.nLength = sizeof( SECURITY_ATTRIBUTES );
    security_attributes.bInheritHandle = TRUE;
    security_attributes.lpSecurityDescriptor = nullptr;
    std::lock_guard< tiny::multithreading::mutex_type > lock( create_process_mutex );
    if( stdin_fd ) {
        if( ( !::CreatePipe( &stdin_rd_p, &stdin_wr_p, &security_attributes, 0 ) ) ||
            ( !::SetHandleInformation( stdin_wr_p, HANDLE_FLAG_INHERIT, 0 ) ) )
            return 0;
    }
    if( stdout_fd ) {
        if( ( !::CreatePipe( &stdout_rd_p, &stdout_wr_p, &security_attributes, 0 ) ) ||
            ( !::SetHandleInformation( stdout_rd_p, HANDLE_FLAG_INHERIT, 0 ) ) )
            return 0;
    }
    if( stderr_fd ) {
        if( ( !::CreatePipe( &stderr_rd_p, &stderr_wr_p, &security_attributes, 0 ) ) ||
            ( !::SetHandleInformation( stderr_rd_p, HANDLE_FLAG_INHERIT, 0 ) ) )
            return 0;
    }
    PROCESS_INFORMATION process_info;
    STARTUPINFO startup_info;
    ::ZeroMemory( &process_info, sizeof( PROCESS_INFORMATION ) );
    ::ZeroMemory( &startup_info, sizeof( STARTUPINFO ) );
    startup_info.cb = sizeof( STARTUPINFO );
    startup_info.hStdInput = stdin_rd_p;
    startup_info.hStdOutput = stdout_wr_p;
    startup_info.hStdError = stderr_wr_p;
    if( stdin_fd || stdout_fd || stderr_fd )
        startup_info.dwFlags |= STARTF_USESTDHANDLES;
    string_type_t process_command = strCommand;
#ifdef MSYS_PROCESS_USE_SH
    size_t pos = 0;
    while( ( pos = process_command.find( '\\', pos ) ) != string_type_t::npos ) {
        process_command.replace( pos, 1, "\\\\\\\\" );
        pos += 4;
    }
    pos = 0;
    while( ( pos = process_command.find( '\"', pos ) ) != string_type_t::npos ) {
        process_command.replace( pos, 1, "\\\"" );
        pos += 2;
    }
    process_command.insert( 0, "sh -c \"" );
    process_command += "\"";
#endif
    BOOL bSuccess = ::CreateProcess( nullptr,
                                     process_command.empty() ? nullptr : &process_command[ 0 ],
                                     nullptr,
                                     nullptr,
                                     TRUE,
                                     0,
                                     nullptr,
                                     strPath.empty() ? nullptr : strPath.c_str(),
                                     &startup_info,
                                     &process_info );
    if( !bSuccess ) {
        ::CloseHandle( process_info.hProcess );
        ::CloseHandle( process_info.hThread );
        return 0;
    }
    else
        ::CloseHandle( process_info.hThread );
    if( stdin_fd )
        ( *stdin_fd ) = stdin_wr_p.detach();
    if( stdout_fd )
        ( *stdout_fd ) = stdout_rd_p.detach();
    if( stderr_fd )
        ( *stderr_fd ) = stderr_rd_p.detach();
    bWasClosed_ = false;
    data_.id = process_info.dwProcessId;
    data_.handle = process_info.hProcess;
    return process_info.dwProcessId;
}
#else   // (defined _WIN32)
pid_type_t process::open( std::function< void() > function ) {
    if( open_stdin )
        stdin_fd = std::unique_ptr< fd_type_t >( new fd_type_t );
    if( read_stdout )
        stdout_fd = std::unique_ptr< fd_type_t >( new fd_type_t );
    if( read_stderr )
        stderr_fd = std::unique_ptr< fd_type_t >( new fd_type_t );
    int stdin_p[ 2 ], stdout_p[ 2 ], stderr_p[ 2 ];
    if( stdin_fd && pipe( stdin_p ) != 0 )
        return -1;
    if( stdout_fd && pipe( stdout_p ) != 0 ) {
        if( stdin_fd ) {
            close( stdin_p[ 0 ] );
            close( stdin_p[ 1 ] );
        }
        return -1;
    }
    if( stderr_fd && pipe( stderr_p ) != 0 ) {
        if( stdin_fd ) {
            close( stdin_p[ 0 ] );
            close( stdin_p[ 1 ] );
        }
        if( stdout_fd ) {
            close( stdout_p[ 0 ] );
            close( stdout_p[ 1 ] );
        }
        return -1;
    }
    pid_type_t pid = fork();
    if( pid < 0 ) {
        if( stdin_fd ) {
            close( stdin_p[ 0 ] );
            close( stdin_p[ 1 ] );
        }
        if( stdout_fd ) {
            close( stdout_p[ 0 ] );
            close( stdout_p[ 1 ] );
        }
        if( stderr_fd ) {
            close( stderr_p[ 0 ] );
            close( stderr_p[ 1 ] );
        }
        return pid;
    }
    else if( pid == 0 ) {
        if( stdin_fd )
            dup2( stdin_p[ 0 ], 0 );
        if( stdout_fd )
            dup2( stdout_p[ 1 ], 1 );
        if( stderr_fd )
            dup2( stderr_p[ 1 ], 2 );
        if( stdin_fd ) {
            close( stdin_p[ 0 ] );
            close( stdin_p[ 1 ] );
        }
        if( stdout_fd ) {
            close( stdout_p[ 0 ] );
            close( stdout_p[ 1 ] );
        }
        if( stderr_fd ) {
            close( stderr_p[ 0 ] );
            close( stderr_p[ 1 ] );
        }
        // based on http://stackoverflow.com/a/899533/3808293
        int fd_max = static_cast< int >( sysconf( _SC_OPEN_MAX ) );  // truncation is safe
        for( int fd = 3; fd < fd_max; fd++ )
            close( fd );
        setpgid( 0, 0 );
        // TODO: See here on how to emulate tty for colors:
        // http://stackoverflow.com/questions/1401002/trick-an-application-into-thinking-its-stdin-is-interactive-not-a-pipe
        // TODO: One solution is: echo "command;exit"|script -q /dev/nullptr
        if( function )
            function();
        _exit( EXIT_FAILURE );
    }
    if( stdin_fd )
        close( stdin_p[ 0 ] );
    if( stdout_fd )
        close( stdout_p[ 1 ] );
    if( stderr_fd )
        close( stderr_p[ 1 ] );
    if( stdin_fd )
        ( *stdin_fd ) = stdin_p[ 1 ];
    if( stdout_fd )
        ( *stdout_fd ) = stdout_p[ 0 ];
    if( stderr_fd )
        ( *stderr_fd ) = stderr_p[ 0 ];
    bWasClosed_ = false;
    data_.id = pid;
    return pid;
}
pid_type_t process::open( const std::string& strCommand, const std::string& strPath ) {
    // original version
    return open( [&strCommand, &strPath] {
        if( !strPath.empty() ) {
            auto path_escaped = strPath;
            size_t pos = 0;
            // based on
            // https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxsxyb7
            while( ( pos = path_escaped.find( '\'', pos ) ) != std::string::npos ) {
                path_escaped.replace( pos, 1, "'\\''" );
                pos += 4;
            }
            ::execl( "/bin/sh", "sh", "-c", ( "cd '" + path_escaped + "' && " + strCommand ).c_str(), nullptr );
        }
        else
            ::execl( "/bin/sh", "sh", "-c", strCommand.c_str(), nullptr );
    } );

    // test version
    //      int fork_rv = fork();
    //      if( fork_rv < 0 )
    //          return -1;
    //      if( fork_rv > 0 ) {
    //          data_.id = fork_rv;
    //          return fork_rv;
    //      }
    //      // we are child
    //      if( ! strPath.empty() ) {
    //        auto path_escaped = strPath;
    //        size_t pos = 0;
    //        // based on
    //        https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxsxyb7
    //        while( ( pos = path_escaped.find( '\'', pos ) ) != std::string::npos ) {
    //          path_escaped.replace( pos, 1, "'\\''" );
    //          pos += 4;
    //        }
    //        ::execl( "/bin/sh", "sh", "-c", ("cd '"+path_escaped+"' && "+strCommand).c_str(), nullptr );
    //      }
    //      else
    //        ::execl( "/bin/sh", "sh", "-c", strCommand.c_str(), nullptr );

    return -1;  // we should never be here
}
#endif  // else from (defined _WIN32)

#if( defined _WIN32 )
void process::async_read() {
    if( data_.id == 0 )
        return;
    if( stdout_fd ) {
        stdout_thread = std::thread( [this]() {
            DWORD n;
            std::unique_ptr< char[] > buf( new char[ buffer_size ] );
            for( ;; ) {
                BOOL bSuccess = ::ReadFile(
                    *stdout_fd, static_cast< CHAR* >( buf.get() ), static_cast< DWORD >( buffer_size ), &n, nullptr );
                if( ( !bSuccess ) || n == 0 )
                    break;
                read_stdout( buf.get(), static_cast< size_t >( n ) );
            }
        } );
    }
    if( stderr_fd ) {
        stderr_thread = std::thread( [this]() {
            DWORD n;
            std::unique_ptr< char[] > buf( new char[ buffer_size ] );
            for( ;; ) {
                BOOL bSuccess = ::ReadFile(
                    *stderr_fd, static_cast< CHAR* >( buf.get() ), static_cast< DWORD >( buffer_size ), &n, nullptr );
                if( ( !bSuccess ) || n == 0 )
                    break;
                read_stderr( buf.get(), static_cast< size_t >( n ) );
            }
        } );
    }
}
#else   // (defined _WIN32)
void process::async_read() {
    if( data_.id <= 0 )
        return;
    if( stdout_fd ) {
        stdout_thread = std::thread( [this]() {
            auto buf = std::unique_ptr< char[] >( new char[ buffer_size ] );
            ssize_t n;
            while( ( n = read( *stdout_fd, buf.get(), buffer_size ) ) > 0 )
                read_stdout( buf.get(), static_cast< size_t >( n ) );
        } );
    }
    if( stderr_fd ) {
        stderr_thread = std::thread( [this]() {
            auto buf = std::unique_ptr< char[] >( new char[ buffer_size ] );
            ssize_t n;
            while( ( n = read( *stderr_fd, buf.get(), buffer_size ) ) > 0 )
                read_stderr( buf.get(), static_cast< size_t >( n ) );
        } );
    }
}
#endif  // else from (defined _WIN32)

#if( defined _WIN32 )
int process::get_exit_status() {
    if( data_.id == 0 )
        return -1;
    DWORD exit_status;
    ::WaitForSingleObject( data_.handle, INFINITE );
    if( !::GetExitCodeProcess( data_.handle, &exit_status ) )
        exit_status = -1;
    {
        std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
        CloseHandle( data_.handle );
        bWasClosed_ = true;
    }
    close_fds();
    return static_cast< int >( exit_status );
}
#else   // (defined _WIN32)
int process::get_exit_status() {
    if( data_.id <= 0 )
        return -1;
    int exit_status;
    ::waitpid( data_.id, &exit_status, 0 );
    {
        std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
        bWasClosed_ = true;
    }
    close_fds();
    if( exit_status >= 256 )
        exit_status = exit_status >> 8;
    return exit_status;
}
#endif  // else from (defined _WIN32)

#if( defined _WIN32 )
void process::close_fds() {
    if( stdout_thread.joinable() )
        stdout_thread.join();
    if( stderr_thread.joinable() )
        stderr_thread.join();
    if( stdin_fd )
        close_stdin();
    if( stdout_fd ) {
        if( ( *stdout_fd ) != nullptr )
            ::CloseHandle( *stdout_fd );
        stdout_fd.reset();
    }
    if( stderr_fd ) {
        if( ( *stderr_fd ) != nullptr )
            ::CloseHandle( *stderr_fd );
        stderr_fd.reset();
    }
}
#else   // (defined _WIN32)
void process::close_fds() {
    if( stdout_thread.joinable() )
        stdout_thread.join();
    if( stderr_thread.joinable() )
        stderr_thread.join();
    if( stdin_fd )
        close_stdin();
    if( stdout_fd ) {
        if( data_.id > 0 )
            close( *stdout_fd );
        stdout_fd.reset();
    }
    if( stderr_fd ) {
        if( data_.id > 0 )
            close( *stderr_fd );
        stderr_fd.reset();
    }
}
#endif  // else from (defined _WIN32)

#if( defined _WIN32 )
bool process::write( const char* bytes, size_t n ) {
    if( !open_stdin )
        throw std::invalid_argument(
            "Can't write to an unopened stdin pipe. Please set open_stdin=true when constructing the process." );
    std::lock_guard< tiny::multithreading::mutex_type > lock( stdin_mutex );
    if( stdin_fd ) {
        DWORD written;
        BOOL bSuccess = ::WriteFile( *stdin_fd, bytes, static_cast< DWORD >( n ), &written, nullptr );
        if( ( !bSuccess ) || written == 0 )
            return false;
        return true;
    }
    return false;
}
#else   // (defined _WIN32)
bool process::write( const char* bytes, size_t n ) {
    if( !open_stdin )
        throw std::invalid_argument(
            "Can't write to an unopened stdin pipe. Please set open_stdin=true when constructing the process." );
    std::lock_guard< tiny::multithreading::mutex_type > lock( stdin_mutex );
    if( stdin_fd ) {
        if(::write( *stdin_fd, bytes, n ) >= 0 )
            return true;
        return false;
    }
    return false;
}
#endif  // else from (defined _WIN32)

#if( defined _WIN32 )
void process::close_stdin() {
    std::lock_guard< tiny::multithreading::mutex_type > lock( stdin_mutex );
    if( stdin_fd ) {
        if( ( *stdin_fd ) != nullptr )
            ::CloseHandle( *stdin_fd );
        stdin_fd.reset();
    }
}
#else   // (defined _WIN32)
void process::close_stdin() {
    std::lock_guard< tiny::multithreading::mutex_type > lock( stdin_mutex );
    if( stdin_fd ) {
        if( data_.id > 0 )
            close( *stdin_fd );
        stdin_fd.reset();
    }
}
#endif  // else from (defined _WIN32)

#if( defined _WIN32 )
// based on http://stackoverflow.com/a/1173396
void process::kill( bool force ) {
    std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
    if( data_.id > 0 && ( !bWasClosed_ ) ) {
        HANDLE snapshot = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
        if( snapshot ) {
            PROCESSENTRY32 process;
            ::ZeroMemory( &process, sizeof( process ) );
            process.dwSize = sizeof( process );
            if(::Process32First( snapshot, &process ) ) {
                do {
                    if( process.th32ParentProcessID == data_.id ) {
                        HANDLE process_handle = ::OpenProcess( PROCESS_TERMINATE, FALSE, process.th32ProcessID );
                        if( process_handle ) {
                            ::TerminateProcess( process_handle, 2 );
                            ::CloseHandle( process_handle );
                        }
                    }
                } while(::Process32Next( snapshot, &process ) );
            }
            ::CloseHandle( snapshot );
        }
        ::TerminateProcess( data_.handle, 2 );
    }
}
// based on http://stackoverflow.com/a/1173396
void process::kill( pid_type_t id, bool force ) {
    if( id == 0 )
        return;
    HANDLE snapshot = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( snapshot ) {
        PROCESSENTRY32 process;
        ::ZeroMemory( &process, sizeof( process ) );
        process.dwSize = sizeof( process );
        if(::Process32First( snapshot, &process ) ) {
            do {
                if( process.th32ParentProcessID == id ) {
                    HANDLE process_handle = ::OpenProcess( PROCESS_TERMINATE, FALSE, process.th32ProcessID );
                    if( process_handle ) {
                        ::TerminateProcess( process_handle, 2 );
                        ::CloseHandle( process_handle );
                    }
                }
            } while(::Process32Next( snapshot, &process ) );
        }
        CloseHandle( snapshot );
    }
    HANDLE process_handle = ::OpenProcess( PROCESS_TERMINATE, FALSE, id );
    if( process_handle )
        ::TerminateProcess( process_handle, 2 );
}
#else   // (defined _WIN32)
void process::kill( bool force ) {
    std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
    if( data_.id > 0 && ( !bWasClosed_ ) ) {
        if( force )
            ::kill( -data_.id, SIGTERM );
        else
            ::kill( -data_.id, SIGINT );
    }
}
void process::kill( pid_type_t id, bool force ) {
    if( id <= 0 )
        return;
    if( force )
        ::kill( -id, SIGTERM );
    else
        ::kill( -id, SIGINT );
}
void process::finish_him() {
    std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
    if( data_.id > 0 && ( !bWasClosed_ ) )
        ::kill( -data_.id, SIGKILL );
}
#endif  // else from (defined _WIN32)

#if( !defined _WIN32 )
volatile bool simplified_unix_process_runner::bIsForkedChild_ = false;
simplified_unix_process_runner::simplified_unix_process_runner( const string_type_t& strCommand,
                                                                const string_type_t& strPath )
: processID_( -1 )
, bWasClosed_( true )
, mutexClose_( "MTX-SIMPLIFIED-UNIX-PROCESS-CLOSE" ) {
    processID_ = fork();
    if( processID_ == 0 ) {
        bIsForkedChild_ = true;
        //::execl( strCommand.c_str(), nullptr );

        //            if( ! strPath.empty() ) {
        //              auto path_escaped = strPath;
        //              size_t pos = 0;
        //              // based on
        //              https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxsxyb7
        //              while( ( pos = path_escaped.find( '\'', pos ) ) != std::string::npos ) {
        //                path_escaped.replace( pos, 1, "'\\''" );
        //                pos += 4;
        //              }
        //              ::execl( "/bin/sh", "sh", "-c", ("cd '"+path_escaped+"' && "+strCommand).c_str(), nullptr );
        //            }
        //            else
        //              ::execl( "/bin/sh", "sh", "-c", strCommand.c_str(), nullptr );

#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        pid_type_t ownChildID = getpid();
        std::cerr << "SUPR: Child process " << ownChildID << " is running.\n";
#endif
        chdir( strPath.c_str() );
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        std::cerr << "SUPR: Child process " << ownChildID << " will execute \"" << strCommand << "\" command.\n";
        int sysRetCode =
#endif
            system( strCommand.c_str() );
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        std::cerr << "SUPR: Child process " << ownChildID << " did execute \"" << strCommand
                  << "\" command. Returned value is " << sysRetCode << ". Will exit\n";
#endif
        ::_Exit( 0 );
    }
    else {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        std::cerr << "SUPR: This process(" << getpid() << ") did started child process " << processID_ << ".\n";
#endif
        bWasClosed_ = false;
    }
}
pid_type_t simplified_unix_process_runner::get_process_id() {  // get the process id of the started process
    return processID_;
}
bool simplified_unix_process_runner::is_closed() const {
    if( processID_ <= 0 || bWasClosed_ )
        return true;
    if( !bWasClosed_ ) {
        // based on
        // https://stackoverflow.com/questions/11785936/how-to-find-if-a-process-is-running-in-clinux
        // https://stackoverflow.com/questions/6898337/determine-programmatically-if-a-program-is-running
        std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
        int ret = ::kill( processID_, 0 );
        if( ret == 0 ) {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
            std::cerr << "SUPR: Process " << processID_ << " is still running.\n";
#endif
            return false;
        }
        if( ret == -1 ) {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
            std::cerr << "SUPR: Process " << processID_ << " was already finished.\n";
#endif
            bWasClosed_ = true;
            processID_ = -1;
            return true;
        }
    }
    return bWasClosed_;
}
int simplified_unix_process_runner::get_exit_status() {
    if( processID_ <= 0 || bWasClosed_ )
        return -1;
    int exit_status;
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Waiting for process " << processID_ << ".\n";
#endif
    ::waitpid( processID_, &exit_status, 0 );
    {
        std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
        bWasClosed_ = true;
    }
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Wait for process " << processID_ << " done.\n";
#endif
    // close_fds();
    if( exit_status >= 256 )
        exit_status = exit_status >> 8;
    return exit_status;
}
void simplified_unix_process_runner::kill( bool force ) {
    std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
    if( processID_ > 0 && ( !bWasClosed_ ) ) {
        int nSignal = force ? SIGTERM : SIGINT;
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        std::cerr << "SUPR: Sending signal " << nSignal << " to process " << processID_ << ".\n";
#endif
        ::kill( -processID_, nSignal );
    }
}
void simplified_unix_process_runner::kill( pid_type_t id, bool force ) {
    if( id <= 0 )
        return;
    int nSignal = force ? SIGTERM : SIGINT;
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Sending signal " << nSignal << " to process " << id << ".\n";
#endif
    ::kill( -id, nSignal );
}
void simplified_unix_process_runner::finish_him() {
    std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
    if( processID_ > 0 && ( !bWasClosed_ ) ) {
        int nSignal = SIGKILL;
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        std::cerr << "SUPR: Sending signal " << nSignal << " to process " << processID_ << ".\n";
#endif
        ::kill( -processID_, nSignal );
    }
}
#endif  // (!defined _WIN32)

#if( !defined _WIN32 )
thread_based_process_controller::thread_based_process_controller(
    const string_type_t& strCommand,
    const string_type_t& strPath,
    thread_based_process_controller::callback_t  // fnProcessDidExit
    )
: mutexClose_( "MTX-THREAD-BASED-PROCESS-CONTROLLER-CLOSE" )
, processID_( -1 )
, bWasClosed_( true )
, strCommand_( strCommand )
, strPath_( strPath )
, thread_( 0 )
, bThreadInitDone_( false )
//        , fnProcessDidExit_( fnProcessDidExit )
{
    run();
}
void thread_based_process_controller::run() {
    std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
    if( !bWasClosed_ )
        return;
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Will create thread.\n";
#endif
    bThreadInitDone_ = false;
    if(::pthread_create( &thread_, nullptr, stat_thread_proc, (void*) this ) ) {
        bWasClosed_ = true;
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        std::cerr << "SUPR: FAILED to create thread.\n";
#endif
    }
    else {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        std::cerr << "SUPR: Did created thread.\n";
#endif
        while( !bThreadInitDone_ ) {
            tiny::sleep_for_milliseconds( 100 );
        }
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        std::cerr << "SUPR: Thread initialized OKay.\n";
#endif
    }
}
void* thread_based_process_controller::stat_thread_proc( void* pThis ) {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Thread proc start.\n";
#endif
    ( (thread_based_process_controller*) pThis )->thread_proc();
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Thread proc finish.\n";
#endif
    ::pthread_exit( nullptr );
    return nullptr;
}
void thread_based_process_controller::thread_proc() {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    pid_type_t pidThread = ( pid_type_t )::syscall( __NR_gettid );
    std::cerr << "SUPR: Entered thread " << pidThread << ".\n";
#endif
    pid_type_t ownChildID = -1;
    {
        bWasClosed_ = false;
        ownChildID = getpid();
        processID_ = ownChildID;
    }
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Child process " << ownChildID << " is running.\n";
#endif
    // chdir( strPath_.c_str() );
    bThreadInitDone_ = true;
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Child process " << ownChildID << " will execute \"" << strCommand_ << "\" command.\n";
    int sysRetCode = 0;
#endif
    try {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
        sysRetCode =
#endif
            system( strCommand_.c_str() );
    }
    catch( ... ) {
    }
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
    std::cerr << "SUPR: Child process " << ownChildID << " did execute \"" << strCommand_
              << "\" command. Returned value is " << sysRetCode << ". Will exit\n";
#endif
    bWasClosed_ = true;
    bThreadInitDone_ = false;
    processID_ = -1;
    thread_ = 0;
}
bool thread_based_process_controller::is_closed() const {
    return bWasClosed_;
}
pid_type_t thread_based_process_controller::get_process_id() {  // get the process id of the started process
    return processID_;
}
// int thread_based_process_controller::get_exit_status() {
//}
void thread_based_process_controller::kill( bool force ) {
    if( thread_ != 0 ) {
        // pthread_t th = thread_;
        try {
            //::pthread_cancel( th );
            // // //::pthread_kill( th, SIGKILL );
            std::string strFind = tiny::tools::replace_all_copy( strCommand_, " 1>/dev/nullptr 2>/dev/nullptr", "" );
            tiny::process_info_list_t lst = tiny::tools::find_processes_by_cmd_line( strFind.c_str() );
            auto cnt = lst.size();
            if( cnt == 1 ) {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
                std::cerr << "SUPR: Found process " << pi.pid_ << " by command line " << strFind << ".\n";
#endif
                const tiny::process_info_t& pi = lst.front();
                auto a_pid = pi.pid_;
                int nSignal = force ? /*SIGTERM*/ SIGKILL : SIGINT;
                ::kill( a_pid, nSignal );
                int exit_status = 0;
                ::waitpid( a_pid, &exit_status, 0 );
                {
                    std::lock_guard< tiny::multithreading::mutex_type > lock( mutexClose_ );
                    thread_ = 0;
                    bThreadInitDone_ = false;
                    bWasClosed_ = true;
                }
            }
            else {
#if( defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__ )
                std::cerr << "SUPR: process was not NOT FOUND by command line " << strFind << ".\n";
#endif
                return;
            }
        }
        catch( ... ) {
        }
    }
}
void thread_based_process_controller::finish_him() {
    //        std::lock_guard < tiny::multithreading::mutex_type > lock( mutexClose_ );
    //        if( processID_ > 0 && (!bWasClosed_) ) {
    //            int nSignal = SIGKILL;
    //#if (defined __EZLO_DEBUG_LOGGING_PROCESS_APIS__)
    //            std::cerr << "SUPR: Sending signal " << nSignal << " to process " << processID_ << ".\n";
    //#endif
    //            ::kill( -processID_, nSignal );
    //        }
}
#endif  // (!defined _WIN32)

#if( !defined _WIN32 )
watch_thread::watch_thread( const string_type_t& strThreadName,
                            const duration_t& sleepTimeBetweenWork,
                            watch_thread::callback_t cbWorker,
                            watch_thread::callback_t cbStarted,
                            watch_thread::callback_t cbStopped,
                            watch_thread::callback_t cbKilled,
                            watch_thread::callback_t cbDestroyed )
: mutexClose_( "RMTX-WATCH-THREAD-CLOSE" )
, strThreadName_( strThreadName )
, sleepTimeBetweenWork_( sleepTimeBetweenWork )
, cbWorker_( cbWorker )
, cbStarted_( cbStarted )
, cbStopped_( cbStopped )
, cbKilled_( cbKilled )
, cbDestroyed_( cbDestroyed )
, thread_( 0 )
, bThreadInitDone_( false )
, bSignalForStop_( false ) {
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Instantiating watch thread \"" << strThreadName_ << "\" with timeout "
              << sleepTimeBetweenWork_.count() << ".\n";
#endif
    run();
}
watch_thread::~watch_thread() {
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Will destroy watch thread \"" << strThreadName_ << "\".\n";
#endif
    kill();
    if( cbDestroyed_ )
        cbDestroyed_( *this );
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Destroyed watch thread \"" << strThreadName_ << "\".\n";
#endif
}
bool watch_thread::run() {
    if( !cbWorker_ )
        return false;
    std::lock_guard< tiny::multithreading::recursive_mutex_type > lock( mutexClose_ );
    if( is_running() )
        return true;
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Starting watch thread \"" << strThreadName_ << "\" with timeout " << sleepTimeBetweenWork_.count()
              << ".\n";
#endif
    bThreadInitDone_ = bSignalForStop_ = false;
    if(::pthread_create( &thread_, nullptr, stat_thread_proc, (void*) this ) ) {
        thread_ = 0;
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
        std::cerr << "FAILED to start watch thread \"" << strThreadName_ << "\".\n";
#endif
        return false;
    }
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Waiting for startup of watch thread \"" << strThreadName_ << "\".\n";
#endif
    waitStarted();
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Started watch thread \"" << strThreadName_ << "\".\n";
#endif
    return true;
}
void* watch_thread::stat_thread_proc( void* pThis ) {
    ( (watch_thread*) pThis )->thread_proc();
    ::pthread_exit( nullptr );
    return nullptr;
}
void watch_thread::thread_proc() {
    //        std::string strThreadName = strThreadName_;
    callback_t cbStarted = cbStarted_, cbStopped = cbStopped_, cbWorker = cbWorker_;
    duration_t sleepTimeBetweenWork = sleepTimeBetweenWork_;
    // pid_type_t pidThread = (pid_type_t)::syscall( __NR_gettid );
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Thread procedure reached for watch thread \"" << strThreadName << "\".\n";
#endif
    {
        bThreadInitDone_ = true;
        if( cbStarted )
            cbStarted( *this );
    }
    for( ; true; ) {
        std::this_thread::sleep_for( sleepTimeBetweenWork );
        if( needsStop() ) {
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
            std::cerr << "Will stop work of watch thread \"" << strThreadName << "\".\n";
#endif
            break;
        }
        cbWorker( *this );
    }  // for( ; true; )
    bThreadInitDone_ = bSignalForStop_ = false;
    if( cbStopped )
        cbStopped( *this );
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Exiting from thread procedure of watch thread \"" << strThreadName << "\".\n";
#endif
}
void watch_thread::waitStarted() {
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Waiting for start of watch thread \"" << strThreadName_ << "\".\n";
#endif
    while( !bThreadInitDone_ )  // no locking neede here
        tiny::sleep_for_milliseconds( 100 );
}
void watch_thread::waitStopped() {
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Waiting for stop of watch thread \"" << strThreadName_ << "\".\n";
#endif
    while( !bSignalForStop_ )  // no locking neede here
        tiny::sleep_for_milliseconds( 100 );
}
void watch_thread::signalForStop() {
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Signaling for stop of watch thread \"" << strThreadName_ << "\".\n";
#endif
    bSignalForStop_ = true;
}
bool watch_thread::needsStop() const {
    return bSignalForStop_;
}
void watch_thread::stop() {
    {
        std::lock_guard< tiny::multithreading::recursive_mutex_type > lock( mutexClose_ );
        if( !is_running() )
            return;
        signalForStop();
    }
    waitStopped();
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
    std::cerr << "Stopped watch thread \"" << strThreadName_ << "\".\n";
#endif
}
void watch_thread::kill() {
    std::lock_guard< tiny::multithreading::recursive_mutex_type > lock( mutexClose_ );
    if( thread_ != 0 ) {
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
        std::cerr << "Will kill watch thread \"" << strThreadName_ << "\".\n";
#endif
        try {
            ::pthread_cancel( thread_ );
        }
        catch( ... ) {
        }
        thread_ = 0;
        bThreadInitDone_ = false;
        if( cbKilled_ )
            cbKilled_( *this );
#if( defined __EZLO_DEBUG_LOGGING_WATCHER_APIS__ )
        std::cerr << "Killed watch thread \"" << strThreadName_ << "\".\n";
#endif
    }
}
bool watch_thread::is_running() const {
    return bThreadInitDone_;
}
#endif  // (!defined _WIN32)

};  // namespace tiny
