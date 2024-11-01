#pragma once

#include "uci_utils/uci_file.h"

namespace HubPlatform {
namespace UciUtils {

class UciNetworkFile final : public UciFile {
public:
    explicit UciNetworkFile( std::string fileName );
    UciNetworkFile();

    bool apply();
};

}  // namespace UciUtils
}  // namespace HubPlatform
