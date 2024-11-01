#if( !defined __TINY_PROCESS_H )
#define __TINY_PROCESS_H 1

// note: process controller is based on https://github.com/eidheim/tiny-process-library

#include <tiny/tiny_utils.h>
#include <tiny/tiny_multithreading.h>

#if( !defined _WIN32 )
#include <sys/wait.h>
#include <pthread.h>
#endif  // (!defined _WIN32)

#include <time.h>

namespace tiny {

#if( defined _WIN32 )
typedef unsigned long pid_type_t;  // process id type
typedef void* fd_type_t;           // file descriptor type
#if( defined UNICODE )
typedef std::wstring string_type_t;
#else
typedef std::string string_type_t;
#endif
#else   // (defined _WIN32)
typedef pid_t pid_type_t;
typedef int fd_type_t;
// unix specific
#endif  // else from (defined _WIN32)
struct process_info_t {
    pid_type_t pid_;
    std::string cmdLine_;
};  // struct process_info_t
typedef std::list< process_info_t > process_info_list_t;

namespace tools {
#if( !defined _WIN32 )
std::string calc_path_of_current_exe();
std::string calc_dir_of_current_exe();  // without '/' at the end of string, empty if fail
#endif                                  // (!defined _WIN32)
extern std::vector< unsigned char > file2vec( const char* strFileName );
extern std::string file2string3vec( const char* strFileName );
extern bool file2stringNoSize( const char* strFileName, std::string& s, int nLimitLineCount = -1 );
extern std::string file2stringNoSize( const char* strFileName, int nLimitLineCount = -1 );
extern bool file2string( const char* strFileName, std::string& s );
extern std::string file2string( const char* strFileName );
extern bool string2file( const char* strFileName, const char* s );
extern bool string2file( const char* strFileName, const std::string& s );
extern string_list_t splitCmdLine( const char* strCmdLine );
extern std::string findCmdLineArgValue( string_list_t lstCmdLine, const char* strArg );
inline std::string findCmdLineArgValue( const char* strCmdLine, const char* strArg ) {
    return findCmdLineArgValue( splitCmdLine( strCmdLine ), strArg );
}
//#if (!defined _WIN32)
extern std::string get_process_dir_entry_path( pid_type_t a_pid );
extern std::string get_process_cmd_line( pid_type_t a_pid, const char* strSep );
extern process_info_list_t find_processes_by_name(
    const char* strExecutableName = nullptr );  // nullptr name to get all)
extern process_info_list_t find_processes_by_cmd_line( const char* strCmdLine );
//#endif // (!defined _WIN32)
};  // namespace tools

class process {  // platform independent class for creating processes
public:
private:
    class Data {
    public:
        Data();
        pid_type_t id;
#if( defined _WIN32 )
        void* handle;
#endif  // (defined _WIN32)
    };

public:
    // Note on Windows: it seems not possible to specify which pipes to redirect.
    // Thus, at the moment, if read_stdout==nullptr, read_stderr==nullptr and open_stdin==false, the stdout, stderr and
    // stdin are sent to the parent process instead.
    process( const string_type_t& strCommand,
             const string_type_t& strPath = string_type_t(),
             std::function< void( const char* bytes, size_t n ) > read_stdout = nullptr,
             std::function< void( const char* bytes, size_t n ) > read_stderr = nullptr,
             bool open_stdin = false,
             size_t buffer_size = 131072 );
#if( !defined _WIN32 )
    // supported on Unix-like systems only.
    process( std::function< void() > function,
             std::function< void( const char* bytes, size_t n ) > read_stdout = nullptr,
             std::function< void( const char* bytes, size_t n ) > read_stderr = nullptr,
             bool open_stdin = false,
             size_t buffer_size = 131072 );
#endif  // (!defined  _WIN32)
    ~process();
    bool is_closed() const;
    pid_type_t get_process_id();                // get the process id of the started process
    int get_exit_status();                      // wait until process is finished, and return exit status
    bool write( const char* bytes, size_t n );  // write to stdin
    bool write( const std::string& a_data );  // write to stdin; convenience function using write(const char *, size_t)
    void close_stdin();  // close stdin; if the process takes parameters from stdin, use this to notify that all
                         // parameters have been sent
    void kill( bool force = false );  // kill the process; force=true is only supported on Unix-like systems
#if( !defined _WIN32 )
    void finish_him();
#endif  // (!defined  _WIN32)
    static void kill( pid_type_t id,
                      bool force = false );  // kill a given process id; use kill( bool force ) instead if possible;
                                             // force=true is only supported on Unix-like systems
private:
    Data data_;
    bool bWasClosed_;
    tiny::multithreading::mutex_type mutexClose_;
    std::function< void( const char* bytes, size_t n ) > read_stdout;
    std::function< void( const char* bytes, size_t n ) > read_stderr;
    std::thread stdout_thread, stderr_thread;
    bool open_stdin;
    tiny::multithreading::mutex_type stdin_mutex;
    size_t buffer_size;
    std::unique_ptr< fd_type_t > stdout_fd, stderr_fd, stdin_fd;
    pid_type_t open( const string_type_t& strCommand, const string_type_t& strPath );
#if( !defined _WIN32 )
    pid_type_t open( std::function< void() > function );
#endif  // (!defined _WIN32)
    void async_read();
    void close_fds();
};  /// class process

#if( !defined _WIN32 )
class simplified_unix_process_runner {
    // based on https://stackoverflow.com/questions/12304949/how-to-create-a-process-in-c-under-linux
public:
    volatile static bool bIsForkedChild_;

private:
    mutable pid_type_t processID_;
    mutable bool bWasClosed_;
    mutable tiny::multithreading::mutex_type mutexClose_;

public:
    simplified_unix_process_runner( const string_type_t& strCommand, const string_type_t& strPath = string_type_t() );
    bool is_closed() const;
    pid_type_t get_process_id();  // get the process id of the started process
    int get_exit_status();
    void kill( bool force = false );  // kill the process
    void finish_him();
    static void kill( pid_type_t id,
                      bool force = false );  // kill a given process id; use kill( bool force ) instead if possible
};                                           /// class simplified_unix_process_runner
#endif                                       // (!defined _WIN32)

#if( !defined _WIN32 )
class thread_based_process_controller {
public:
    typedef thread_based_process_controller my_t;
    typedef thread_based_process_controller& my_rt;
    typedef std::function< void( my_rt ) > callback_t;

private:
    mutable tiny::multithreading::mutex_type mutexClose_;
    mutable volatile pid_type_t processID_;
    mutable volatile bool bWasClosed_ : 1;
    string_type_t strCommand_, strPath_;
    pthread_t thread_;
    mutable volatile bool bThreadInitDone_ : 1;
    static void* stat_thread_proc( void* pThis );
    void thread_proc();
    // callback_t fnProcessDidExit_;
public:
    thread_based_process_controller( const string_type_t& strCommand,
                                     const string_type_t& strPath = string_type_t(),
                                     callback_t fnProcessDidExit = callback_t() );
    void run();
    bool is_closed() const;
    pid_type_t get_process_id();  // get the process id of the started process
    // int get_exit_status();
    void kill( bool force = false );  // kill the process
    void finish_him();
    const string_type_t& get_command() const {
        return strCommand_;
    }
    const string_type_t& get_path() const {
        return strPath_;
    }
};      /// class thread_based_process_controller
#endif  // (!defined _WIN32)

#if( !defined _WIN32 )
class watch_thread {
public:
    typedef watch_thread my_t;
    typedef watch_thread& my_rt;
    typedef std::function< void( my_rt ) > callback_t;
    typedef std::chrono::milliseconds duration_t;

private:
    tiny::multithreading::recursive_mutex_type mutexClose_;
    string_type_t strThreadName_;
    duration_t sleepTimeBetweenWork_;
    callback_t cbWorker_, cbStarted_, cbStopped_, cbKilled_, cbDestroyed_;
    pthread_t thread_;
    mutable volatile bool bThreadInitDone_ : 1;
    mutable volatile bool bSignalForStop_ : 1;
    static void* stat_thread_proc( void* pThis );
    void thread_proc();
    void waitStarted();
    void waitStopped();
    void signalForStop();
    bool needsStop() const;

public:
    watch_thread( const string_type_t& strThreadName,
                  const duration_t& sleepTimeBetweenWork,
                  callback_t cbWorker,
                  callback_t cbStarted = callback_t(),
                  callback_t cbStopped = callback_t(),
                  callback_t cbKilled = callback_t(),
                  callback_t cbDestroyed = callback_t() );
    ~watch_thread();
    bool run();
    void stop();
    void kill();
    bool is_running() const;
};      /// class watch_thread
#endif  // (!defined _WIN32)

};  // namespace tiny

#endif  /// (!defined __TINY_PROCESS_H)
