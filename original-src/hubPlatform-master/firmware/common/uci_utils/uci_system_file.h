#pragma once

#include "uci_utils/uci_file.h"

namespace HubPlatform {
namespace UciUtils {

class UciSystemFile final : public UciFile {
public:
    explicit UciSystemFile( std::string fileName );
    UciSystemFile();

    bool isTimezoneDefault() const;
    std::string getHostname() const;
    std::string getTimezone() const;

    bool setHostname( const std::string& hostname );
    bool setTimeZone( const std::string& zonename );
    bool apply();

private:
    UciSettings::iterator getSystemSection();
    UciSettings::const_iterator getSystemSection() const;
};

}  // namespace UciUtils
}  // namespace HubPlatform
