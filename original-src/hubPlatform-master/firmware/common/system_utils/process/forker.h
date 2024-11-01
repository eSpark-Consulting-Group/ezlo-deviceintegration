#pragma once
#include <string>
#include <mutex>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <exception>

namespace HubPlatform {
namespace Process {

void daemonize();

class ForkError : public std::exception {
public:
    explicit ForkError( const std::string& error ) noexcept;
    virtual const char* what() const _GLIBCXX_USE_NOEXCEPT override;

private:
    const std::string m_error;
};

class Forker {
public:
    using Filename = std::string;
    using ProcessJob = std::function< void( const Filename& errorsOutputFile ) >;

    enum class Status { Ok, TermSignal, ForkFailure, UnknownError };

    struct FileAutoRemover {
        FileAutoRemover( std::string fileName );
        FileAutoRemover( const FileAutoRemover& ) = delete;
        FileAutoRemover( FileAutoRemover&& ) noexcept;
        ~FileAutoRemover();
        FileAutoRemover& operator=( const FileAutoRemover& ) = delete;
        FileAutoRemover& operator=( FileAutoRemover&& ) = delete;
        const std::string& getFileName() const;

    private:
        std::string m_fileName;
    };
    using OnDeamonStart = std::function< void( pid_t ) >;
    using OnProcessCompletion = std::function< void( pid_t, Status status, int exitCode, const std::string& error ) >;
    using Instance = std::shared_ptr< Forker >;

    Forker( const Forker& ) = delete;
    Forker( Forker&& ) = delete;
    ~Forker();

    static Instance getInstance();

    /// @brief after succesful fork function splits on two function are lives with in two
    /// different processes. What process is can be identified by pid value.
    /// if pid > 0 it is parent process and pid equals to pid of child process
    /// if pid == 0 it is child process
    /// if pid < 0 child is not created
    pid_t fork( const std::string& shellCommand, OnProcessCompletion onProcessCompletion );
    pid_t fork( ProcessJob job, OnProcessCompletion onProcessCompletion );

    static pid_t fork( const std::string& shellCommand );
    static void kill( pid_t pid, int signal );

private:
    Forker() = default;
    static void execCommand( const std::string& shellCommand );
    void waitForProcess();

    struct ProcessData {
        OnProcessCompletion onProcessCompletion;
        FileAutoRemover errorFileRemover;
    };

    mutable std::mutex m_mutex;
    std::map< pid_t, ProcessData > m_processes;
    std::thread m_thread;
    std::atomic_bool m_isRunning;

    static constexpr int ExeclExitError = 127;  // exit code of a child process if execl() is failed.
};

}  // namespace Process
}  // namespace HubPlatform
