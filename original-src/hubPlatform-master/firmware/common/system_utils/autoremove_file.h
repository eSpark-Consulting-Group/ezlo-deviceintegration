#pragma once

#include <string>

namespace HubPlatform {
namespace filesystem {

class AutoremoveFile {
public:
    AutoremoveFile( std::string filename, bool autoremove = true );
    ~AutoremoveFile();
    void setAutoremove( bool autoremove );

private:
    const std::string m_filename;
    bool m_autoremove = true;
};

}  // namespace filesystem
}  // namespace HubPlatform
