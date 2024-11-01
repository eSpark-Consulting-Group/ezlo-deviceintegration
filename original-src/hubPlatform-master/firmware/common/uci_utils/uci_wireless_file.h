#pragma once

#include "uci_utils/uci_file.h"

namespace HubPlatform {
namespace UciUtils {

class UciWirelessFile final : public UciFile {
public:
    explicit UciWirelessFile( std::string fileName );
    UciWirelessFile();

    bool apply( bool hard = false );
};

}  // namespace UciUtils
}  // namespace HubPlatform
