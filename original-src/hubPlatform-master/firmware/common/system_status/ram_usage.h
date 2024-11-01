#pragma once

#include <map>
#include <vector>

namespace HubPlatform {

namespace SystemUtils {
namespace ProcFS {

class MemoryInfo {
public:
    struct ProcessStatus {
        std::string name;
        std::string state;
        int64_t memoryTotal;
        int64_t vmRss;
        int64_t rssAnon;
    };

public:
    MemoryInfo();
    int parseMeminfo();
    bool hasFlashInfo();
    bool hasProcessInfo();
    static std::map< std::string, std::string > parseProcStatus( std::istream& statusFile );
    std::tuple< bool, ProcessStatus > fetchProcessMemoryUsage( const char* pid );
    std::tuple< bool, ProcessStatus > fetchProcessMemoryUsage( const char* pid, std::istream& procStream );

private:
    void calculateVolumeSize( const struct mntent* fs );
    bool getProcessMemoryUsage( const char* pid, size_t filterOutLowMemoryUsageKb = 1024 );
    void processDir( const struct dirent* dir );
    static std::string getProcessName( const std::string& pid );

public:
    // Values from /proc/meminfo, in KiB or converted to MiB.
    long memTotalKib;
    int64_t memAvailableKib;  // -1 means no data available

    // Calculated percentages
    int memAvailablePercent;  // percent of total memory that is available

    std::map< std::string, std::vector< unsigned long > > flashDirectories;

    using ProcessPid = std::string;
    using ProcessesMemoryUsage = std::multimap< ProcessPid, ProcessStatus >;
    ProcessesMemoryUsage processList;
};

}  // namespace ProcFS
}  // namespace SystemUtils
}  // namespace HubPlatform
